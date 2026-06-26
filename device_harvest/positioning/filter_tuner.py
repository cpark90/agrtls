"""
실시간 필터 비교 도구 (대화형).
UWB 태그로부터 UDP로 수신한 실제 range 값을 원본 그대로,
그리고 여러 필터를 적용한 결과와 함께 한 그래프에 겹쳐 비교합니다.
각 필터의 변수는 슬라이더로 실시간 조정할 수 있습니다.

비교 필터:
  - Kalman    : 1D 칼만 + 아웃라이어 게이팅 (Q, R, gate σ)
  - EMA       : 지수이동평균 (펌웨어 filterValue와 동일 계열, N)
  - MovingAvg : 단순 이동평균 (윈도우 W)

실행:
    python filter_tuner.py
필요 패키지: PyQt5, pyqtgraph, numpy
데이터: config.py 의 LISTEN_PORT 로 들어오는 "RANGE,addr,range,rxpow"
"""

import sys
import time
from collections import deque

import numpy as np
import pyqtgraph as pg
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QSlider, QGroupBox, QCheckBox, QComboBox,
)

import config
from network import UdpReceiver
from kalman import KalmanFilter1D


# ==================== 스트리밍 필터 ====================
class StreamingEMA:
    """지수이동평균 (펌웨어 filterValue와 동일 계열). 한 샘플씩 갱신."""
    def __init__(self, n_elements):
        self.set_n(n_elements)
        self.prev = None

    def set_n(self, n_elements):
        self.k = 2.0 / (float(n_elements) + 1.0)

    def update(self, x):
        if self.prev is None:
            self.prev = x
        else:
            self.prev = self.k * x + (1.0 - self.k) * self.prev
        return self.prev


class StreamingMovingAvg:
    """단순 이동평균 (과거 window개). 한 샘플씩 갱신."""
    def __init__(self, window):
        self.window = max(1, int(window))
        self.buf = deque()
        self.csum = 0.0

    def set_window(self, window):
        self.window = max(1, int(window))
        while len(self.buf) > self.window:
            self.csum -= self.buf.popleft()

    def update(self, x):
        self.buf.append(x); self.csum += x
        while len(self.buf) > self.window:
            self.csum -= self.buf.popleft()
        return self.csum / len(self.buf)


class DeviceFilters:
    """한 디바이스(앵커)의 원본 버퍼 + 3개 필터 스트림 + 곡선."""
    def __init__(self, dev_id, plot):
        self.dev_id = dev_id
        # 시간/값 버퍼 (이동 창)
        self.t = deque()
        self.raw = deque()
        self.kf_out = deque()
        self.ema_out = deque()
        self.ma_out = deque()
        self.kf_outlier_t = deque()
        self.kf_outlier_v = deque()

        # 필터 인스턴스 (파라미터는 redraw 직전 갱신)
        self.kf = KalmanFilter1D(config.RANGE_Q, config.RANGE_R, config.GATE_SIGMA)
        self.reject_streak = 0
        self.ema = StreamingEMA(15)
        self.ma = StreamingMovingAvg(10)

        # 곡선
        self.c_raw = plot.plot(pen=pg.mkPen((150, 150, 150, 130), width=1), name=f"{dev_id} raw")
        self.c_kf = plot.plot(pen=pg.mkPen((0, 220, 0), width=2), name=f"{dev_id} Kalman")
        self.c_ema = plot.plot(pen=pg.mkPen((60, 140, 255), width=2), name=f"{dev_id} EMA")
        self.c_ma = plot.plot(pen=pg.mkPen((255, 170, 0), width=2), name=f"{dev_id} MovingAvg")
        self.c_out = plot.plot(pen=None, symbol='x',
            symbolPen=pg.mkPen((255, 60, 60), width=2), symbolBrush=(255, 60, 60),
            symbolSize=9, name=f"{dev_id} KF outlier")

    def add_sample(self, t, rng):
        """새 원본 샘플 1개를 받아 모든 필터를 한 스텝 갱신."""
        # Kalman (check → accept/reject)
        is_out = self.kf.check_outlier(rng)
        if is_out and self.reject_streak < config.MAX_REJECT_STREAK:
            self.kf.reject()
            self.reject_streak += 1
            self.kf_outlier_t.append(t); self.kf_outlier_v.append(rng)
        else:
            self.kf.accept(rng)
            self.reject_streak = 0
        kf_val = self.kf.x if self.kf.x is not None else rng

        ema_val = self.ema.update(rng)
        ma_val = self.ma.update(rng)

        self.t.append(t); self.raw.append(rng)
        self.kf_out.append(kf_val)
        self.ema_out.append(ema_val)
        self.ma_out.append(ma_val)

    def trim(self, cutoff):
        def drop(tq, *vqs):
            while tq and tq[0] < cutoff:
                tq.popleft()
                for vq in vqs:
                    if vq: vq.popleft()
        drop(self.t, self.raw, self.kf_out, self.ema_out, self.ma_out)
        # 아웃라이어 버퍼는 (t,v) 쌍 별도 정리
        while self.kf_outlier_t and self.kf_outlier_t[0] < cutoff:
            self.kf_outlier_t.popleft(); self.kf_outlier_v.popleft()

    def update_params(self, q, r, g, ema_n, ma_w):
        # 칼만은 파라미터만 교체 (상태 x/p는 유지해 연속성 보존)
        self.kf.q = q; self.kf.r = r; self.kf.gate_sigma = g
        self.ema.set_n(ema_n)
        self.ma.set_window(ma_w)

    def redraw(self, show):
        T = list(self.t)
        self.c_raw.setData(T, list(self.raw))
        self.c_kf.setData(T, list(self.kf_out) if show['kf'] else [])
        self.c_ema.setData(T, list(self.ema_out) if show['ema'] else [])
        self.c_ma.setData(T, list(self.ma_out) if show['ma'] else [])
        if show['out']:
            self.c_out.setData(list(self.kf_outlier_t), list(self.kf_outlier_v))
        else:
            self.c_out.setData([], [])

    def set_visible(self, visible):
        for c in (self.c_raw, self.c_kf, self.c_ema, self.c_ma, self.c_out):
            c.setVisible(visible)

    def recent_rmse_vs_kalman(self):
        """참값이 없으니, 각 필터가 칼만(가장 강한 필터) 대비 얼마나 다른지 참고용 지표."""
        if len(self.raw) < 5:
            return None
        raw = np.array(self.raw); kf = np.array(self.kf_out)
        return float(np.sqrt(np.mean((raw - kf) ** 2)))


# ==================== 슬라이더 위젯 ====================
class LabeledSlider(QWidget):
    def __init__(self, name, vmin, vmax, step, init, on_change, fmt="{:.3f}"):
        super().__init__()
        self.name = name; self.vmin = vmin; self.step = step
        self.fmt = fmt; self.on_change = on_change
        n_steps = int(round((vmax - vmin) / step))
        init_int = int(round((init - vmin) / step))
        lay = QHBoxLayout(self); lay.setContentsMargins(2, 2, 2, 2)
        self.label = QLabel(); self.label.setMinimumWidth(150)
        self.slider = QSlider(Qt.Horizontal)
        self.slider.setMinimum(0); self.slider.setMaximum(n_steps)
        self.slider.setValue(init_int)
        self.slider.valueChanged.connect(self._changed)
        lay.addWidget(self.label); lay.addWidget(self.slider)
        self._update_label()

    def value(self):
        return self.vmin + self.slider.value() * self.step

    def _update_label(self):
        self.label.setText(f"{self.name} = {self.fmt.format(self.value())}")

    def _changed(self):
        self._update_label()
        self.on_change()


# ==================== 메인 창 ====================
class FilterTuner(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("UWB Range Filter Comparison (live UDP)")
        self.resize(1200, 750)

        self.receiver = UdpReceiver()
        self.start_time = time.time()
        self.devices = {}          # dev_id -> DeviceFilters
        self.selected = "ALL"      # 표시할 디바이스 ("ALL" 또는 특정 id)

        cw = QWidget(); self.setCentralWidget(cw)
        root = QHBoxLayout(cw)

        # 그래프
        self.win = pg.GraphicsLayoutWidget()
        root.addWidget(self.win, stretch=4)
        self.plot = self.win.addPlot(title="Range: Raw vs Filters (live)")
        self.plot.setLabel('left', 'Range', units='m')
        self.plot.setLabel('bottom', 'Time', units='s')
        self.plot.showGrid(x=True, y=True)
        self.plot.addLegend()

        # 컨트롤 패널
        panel = QWidget(); root.addWidget(panel, stretch=2)
        pl = QVBoxLayout(panel)

        # 디바이스 선택
        dev_box = QGroupBox("디바이스 선택")
        dvl = QVBoxLayout(dev_box)
        self.dev_combo = QComboBox()
        self.dev_combo.addItem("ALL")
        self.dev_combo.currentTextChanged.connect(self._on_dev_changed)
        dvl.addWidget(self.dev_combo)
        pl.addWidget(dev_box)

        # 표시 토글
        vis_box = QGroupBox("표시 (Show/Hide)")
        vvl = QVBoxLayout(vis_box)
        self.cb_kf = self._toggle("Kalman", True, vvl)
        self.cb_ema = self._toggle("EMA", True, vvl)
        self.cb_ma = self._toggle("MovingAvg", True, vvl)
        self.cb_out = self._toggle("KF outliers", True, vvl)
        pl.addWidget(vis_box)

        # Kalman
        kf_box = QGroupBox("Kalman"); kl = QVBoxLayout(kf_box)
        self.s_q = LabeledSlider("Q (process)", 0.001, 1.0, 0.001, config.RANGE_Q, self._noop)
        self.s_r = LabeledSlider("R (measure)", 0.01, 5.0, 0.01, config.RANGE_R, self._noop)
        self.s_g = LabeledSlider("gate σ", 1.0, 8.0, 0.1, config.GATE_SIGMA, self._noop, fmt="{:.1f}")
        for s in (self.s_q, self.s_r, self.s_g): kl.addWidget(s)
        pl.addWidget(kf_box)

        # EMA
        ema_box = QGroupBox("EMA (지수이동평균)"); el = QVBoxLayout(ema_box)
        self.s_ema_n = LabeledSlider("N elements", 1.0, 60.0, 1.0, 15.0, self._noop, fmt="{:.0f}")
        el.addWidget(self.s_ema_n); pl.addWidget(ema_box)

        # MovingAvg
        ma_box = QGroupBox("MovingAvg"); ml = QVBoxLayout(ma_box)
        self.s_ma_w = LabeledSlider("window W", 1.0, 60.0, 1.0, 10.0, self._noop, fmt="{:.0f}")
        ml.addWidget(self.s_ma_w); pl.addWidget(ma_box)

        self.info = QLabel(); self.info.setStyleSheet("font-family: monospace;")
        pl.addWidget(self.info)
        pl.addStretch(1)

        # 타이머 루프
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_loop)
        self.timer.start(20)

    # ---- 헬퍼 ----
    def _toggle(self, text, checked, layout):
        c = QCheckBox(text); c.setChecked(checked)
        c.stateChanged.connect(self._noop)
        layout.addWidget(c)
        return c

    def _noop(self):
        # 슬라이더/토글 변경은 다음 프레임 redraw에서 반영되므로 여기선 아무것도 안 함
        pass

    def _on_dev_changed(self, text):
        self.selected = text
        # 표시 대상만 보이게
        for dev_id, dev in self.devices.items():
            dev.set_visible(self.selected == "ALL" or self.selected == dev_id)

    def _get_device(self, dev_id):
        if dev_id not in self.devices:
            dev = DeviceFilters(dev_id, self.plot)
            self.devices[dev_id] = dev
            self.dev_combo.addItem(dev_id)
            # 현재 선택 정책에 맞춰 가시성 설정
            dev.set_visible(self.selected == "ALL" or self.selected == dev_id)
        return self.devices[dev_id]

    # ---- 매 프레임 ----
    def update_loop(self):
        # 1) 수신 + 필터 갱신
        for dev_id, rng, rx in self.receiver.poll():
            t = time.time() - self.start_time
            self._get_device(dev_id).add_sample(t, rng)

        now = time.time() - self.start_time
        cutoff = now - config.WINDOW_TIME_SEC

        # 2) 현재 슬라이더 값으로 필터 파라미터 갱신 + 정리 + 그리기
        q, r, g = self.s_q.value(), self.s_r.value(), self.s_g.value()
        ema_n, ma_w = self.s_ema_n.value(), self.s_ma_w.value()
        show = {
            'kf': self.cb_kf.isChecked(),
            'ema': self.cb_ema.isChecked(),
            'ma': self.cb_ma.isChecked(),
            'out': self.cb_out.isChecked(),
        }

        for dev in self.devices.values():
            dev.update_params(q, r, g, ema_n, ma_w)
            dev.trim(cutoff)
            dev.redraw(show)

        # 3) 시간축 범위
        if self.devices:
            self.plot.setXRange(cutoff, now, padding=0)

        # 4) 정보 표시 (선택된 디바이스의 최근 값)
        self._update_info()

    def _update_info(self):
        lines = ["device : samples  raw   kalman  (지터: raw-kf RMS)"]
        for dev_id, dev in self.devices.items():
            if self.selected != "ALL" and self.selected != dev_id:
                continue
            if not dev.raw:
                continue
            jitter = dev.recent_rmse_vs_kalman()
            js = f"{jitter:.3f}" if jitter is not None else "  -"
            lines.append(f"{dev_id:>6} : {len(dev.raw):>6}  "
                         f"{dev.raw[-1]:.2f}  {dev.kf_out[-1]:.2f}   {js}")
        self.info.setText("\n".join(lines))

    def closeEvent(self, event):
        self.receiver.close()
        event.accept()


def main():
    app = QApplication(sys.argv)
    w = FilterTuner()
    w.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
