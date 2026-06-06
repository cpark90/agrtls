"""
시각화 공통 베이스.
2D/3D가 공유하는 부분을 담습니다:
- UDP 수신 루프
- 앵커 트랙 생성/관리 (거리·RX 그래프)
- 시간 동기화 기반 측위 (차원은 서브클래스가 지정)
맵 뷰 구성과 위치 표시는 서브클래스가 구현합니다.
"""

import time
import numpy as np
import pyqtgraph as pg
from PyQt5.QtCore import QTimer
from PyQt5.QtWidgets import QMainWindow, QVBoxLayout, QWidget

import config
from network import UdpReceiver
from anchor_track import AnchorTrack, make_colors
from multilateration import multilaterate, PosKalmanND


class BaseVisualizer(QMainWindow):
    """dim(2 또는 3)을 받아 차원에 맞춰 동작하는 공통 시각화 베이스."""

    def __init__(self, dim, title):
        super().__init__()
        self.dim = dim
        self.anchors = config.anchors_for_dim(dim)      # {id: (x,y[,z])}
        self.min_anchors = config.min_anchors_for_dim(dim)

        self.setWindowTitle(title)
        self.resize(1100, 800)

        self.receiver = UdpReceiver()
        self.start_time = time.time()
        self.tracks = {}

        # 색상 미리 배정
        ids = list(self.anchors.keys())
        palette = make_colors(len(ids))
        self.color_map = {dev_id: palette[i] for i, dev_id in enumerate(ids)}
        self._extra_color_idx = 0

        # 위치 추정 상태 (초기값: 앵커 무게중심)
        if self.anchors:
            cen = np.mean(np.array(list(self.anchors.values())), axis=0)
        else:
            cen = np.zeros(dim)
        self.pos_est = cen.astype(float)
        self.pos_kf = PosKalmanND(dim)
        self.pos_hist = []  # (t, *coords) 평활화된 궤적

        self._build_common_ui()
        self._build_map_ui()    # 서브클래스 구현

        self.timer = QTimer()
        self.timer.timeout.connect(self.update_data)
        self.timer.start(20)

    # ---------- 공통 UI: 거리 / RX 그래프 ----------
    def _build_common_ui(self):
        cw = QWidget()
        self.setCentralWidget(cw)
        layout = QVBoxLayout(cw)
        self.win = pg.GraphicsLayoutWidget()
        layout.addWidget(self.win)

        self.p1 = self.win.addPlot(row=0, col=0, title="Range per Anchor (Raw vs Kalman)")
        self.p1.setLabel('left', 'Range', units='m')
        self.p1.setLabel('bottom', 'Time', units='s')
        self.p1.showGrid(x=True, y=True)
        self.p1.addLegend()

        self.p2 = self.win.addPlot(row=0, col=1, title="RX Power per Anchor")
        self.p2.setLabel('left', 'RX Power')
        self.p2.setLabel('bottom', 'Time', units='s')
        self.p2.showGrid(x=True, y=True)
        self.p2.addLegend()
        self.p2.setXLink(self.p1)

    # ---------- 서브클래스가 구현 ----------
    def _build_map_ui(self):
        raise NotImplementedError

    def _update_map(self, cutoff):
        raise NotImplementedError

    # ---------- 앵커 트랙 관리 ----------
    def get_track(self, dev_id):
        if dev_id not in self.tracks:
            if dev_id in self.anchors:
                pos = self.anchors[dev_id]
                color = self.color_map[dev_id]
            else:
                print(f"[warn] unknown anchor id {dev_id} (좌표 미등록 → 측위 제외)")
                pos = tuple([0.0] * self.dim)
                shade = 110 + (self._extra_color_idx * 40) % 120
                color = (shade, shade, shade)
                self._extra_color_idx += 1
            self.tracks[dev_id] = AnchorTrack(dev_id, pos, color, self.p1, self.p2)
        return self.tracks[dev_id]

    # ---------- 매 프레임 루프 ----------
    def update_data(self):
        for dev_id, rng, rx in self.receiver.poll():
            t = time.time() - self.start_time
            self.get_track(dev_id).process(rng, rx, t)

        now = time.time() - self.start_time
        self.solve_position(now)
        self.redraw(now)

    def solve_position(self, now):
        # 1) 기준 시각(t_ref): 좌표 등록된 앵커들 중 가장 최근 수신 시각
        t_ref = None
        for dev_id, tr in self.tracks.items():
            if dev_id not in self.anchors:
                continue
            if tr.last_update_t is not None:
                if t_ref is None or tr.last_update_t > t_ref:
                    t_ref = tr.last_update_t
        if t_ref is None or now - t_ref > config.RANGE_TIMEOUT_SEC:
            return

        # 2) t_ref 기준 동기화 창 안의 앵커 거리만 수집
        pos_list, rng_list = [], []
        for dev_id, tr in self.tracks.items():
            if dev_id not in self.anchors:
                continue
            r, t_used, dt = tr.range_at(t_ref)
            if r is not None and r > 0:
                pos_list.append(tr.pos[:self.dim])
                rng_list.append(r)

        if len(rng_list) < self.min_anchors:
            return

        anchor_pos = np.array(pos_list)
        ranges = np.array(rng_list)
        raw_pos, rms = multilaterate(anchor_pos, ranges, self.pos_est)

        if not np.all(np.isfinite(raw_pos)):
            return
        self.pos_est = raw_pos
        sm = self.pos_kf.update(raw_pos)
        self.pos_hist.append((t_ref, *sm))

    def redraw(self, now):
        cutoff = now - config.WINDOW_TIME_SEC
        for tr in self.tracks.values():
            tr.trim_redraw(cutoff)
            tr.trim_hist(cutoff)

        # 시간축 범위
        latest = None
        for tr in self.tracks.values():
            lt = tr.last_time()
            if lt is not None and (latest is None or lt > latest):
                latest = lt
        if latest is not None:
            self.p1.setXRange(latest - config.WINDOW_TIME_SEC, latest, padding=0)

        # 궤적 이동 창 적용 후 맵 갱신
        self.pos_hist = [h for h in self.pos_hist if h[0] >= cutoff]
        self._update_map(cutoff)

    def closeEvent(self, event):
        self.receiver.close()
        event.accept()

    # ---------- 맵 축 자동 범위 헬퍼 ----------
    def _anchor_axis_bounds(self):
        """앵커 좌표 범위 + 여유. 반환: dict {axis_index: (min, max)}."""
        bounds = {}
        if self.anchors:
            arr = np.array(list(self.anchors.values()))
            m = config.MAP_MARGIN_M
            for i in range(self.dim):
                bounds[i] = (arr[:, i].min() - m, arr[:, i].max() + m)
        return bounds
