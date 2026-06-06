"""
앵커 트랙 모듈.
앵커(short address) 하나가 가지는 모든 것을 묶습니다:
range/rx 칼만 필터 2개, 데이터 버퍼, 그래프 곡선, 측위 입력용 최신 거리.
색상은 앵커 개수만큼 색상환에서 균등 분배해 생성합니다.
"""

import colorsys
import numpy as np
import pyqtgraph as pg

from kalman import KalmanFilter1D
import config


def make_colors(n):
    """앵커 개수 n만큼 색상환(HSV)에서 균등 분배한 구분 색을 생성.
    앵커가 10개, 20개로 늘어도 서로 구분됩니다."""
    colors = []
    n = max(n, 1)
    for i in range(n):
        h = i / n                                  # 색상(Hue) 균등 분배
        r, g, b = colorsys.hsv_to_rgb(h, 0.65, 0.95)
        colors.append((int(r * 255), int(g * 255), int(b * 255)))
    return colors


class AnchorTrack:
    """앵커 하나의 거리/RX 필터 + 버퍼 + 곡선 + 최신 거리(측위 입력)."""

    def __init__(self, dev_id, pos, color, p1, p2):
        self.dev_id = dev_id
        self.pos = np.array(pos, dtype=float)   # 앵커 좌표 (x,y,z)
        self.color = color

        self.kf_range = KalmanFilter1D(config.RANGE_Q, config.RANGE_R, config.GATE_SIGMA)
        self.kf_rx = KalmanFilter1D(config.RX_Q, config.RX_R, config.GATE_SIGMA)
        self.reject_streak = 0

        # 측위용 최신값 + 타임스탬프가 찍힌 거리 이력
        self.last_range = None        # 칼만 필터링된 최신 거리
        self.last_update_t = None     # 마지막 정상 갱신 시각
        # (t, filtered_range) 쌍의 이력. 측위 시 기준 시각에 가까운 값을 고르는 데 사용.
        self.range_hist = []

        # 그래프 버퍼
        self.times, self.ranges, self.ranges_f = [], [], []
        self.tx_times, self.tx_vals, self.tx_f = [], [], []
        self.out_t, self.out_r, self.out_x = [], [], []

        raw_pen = pg.mkPen((color[0], color[1], color[2], 90), width=1)
        kf_pen = pg.mkPen(color, width=2)

        self.c_range_raw = p1.plot(pen=raw_pen)
        self.c_range_kf = p1.plot(pen=kf_pen, name=f"{dev_id}")
        self.c_range_out = p1.plot(pen=None, symbol='x',
            symbolPen=pg.mkPen(color, width=2), symbolBrush=color, symbolSize=9)

        self.c_tx_raw = p2.plot(pen=raw_pen)
        self.c_tx_kf = p2.plot(pen=kf_pen, name=f"{dev_id}")
        self.c_tx_out = p2.plot(pen=None, symbol='x',
            symbolPen=pg.mkPen(color, width=2), symbolBrush=color, symbolSize=9)

    def process(self, rng, rx, t):
        """새 측정 1개 처리. range / rx 채널을 독립적으로 아웃라이어 판단.
        (RX가 잠깐 튀어도 멀쩡한 거리 샘플은 측위에 그대로 씀)"""
        out_r = self.kf_range.check_outlier(rng)
        out_x = self.kf_rx.check_outlier(rx)

        # range 처리
        if out_r and self.reject_streak < config.MAX_REJECT_STREAK:
            self.kf_range.reject()
            range_is_out = True
        else:
            r_f = self.kf_range.accept(rng)
            range_is_out = False

        # rx 처리 (range와 독립)
        if out_x and self.reject_streak < config.MAX_REJECT_STREAK:
            self.kf_rx.reject()
            rx_is_out = True
        else:
            x_f = self.kf_rx.accept(rx)
            rx_is_out = False

        # 연속 거부 카운터 (둘 다 정상일 때만 리셋)
        if range_is_out or rx_is_out:
            self.reject_streak += 1
        else:
            self.reject_streak = 0

        # 측위 입력: range가 정상일 때만 갱신
        if not range_is_out:
            self.last_range = r_f
            self.last_update_t = t
            self.range_hist.append((t, r_f))   # 타임스탬프와 함께 이력에 기록
            self.times.append(t); self.ranges.append(rng); self.ranges_f.append(r_f)
        else:
            self.out_t.append(t); self.out_r.append(rng); self.out_x.append(rx)

        if not rx_is_out:
            self.tx_times.append(t); self.tx_vals.append(rx); self.tx_f.append(x_f)

    def range_at(self, t_ref):
        """기준 시각 t_ref에 가장 가까운 거리값을 반환.
        동기화 창(SYNC_WINDOW_SEC) 밖이면 None.
        반환: (거리, 그 값의 실제 시각, |시간차|) 또는 (None, None, None)."""
        if not self.range_hist:
            return None, None, None
        # 이력에서 t_ref와 시간차가 가장 작은 항목 선택
        best = min(self.range_hist, key=lambda tr: abs(tr[0] - t_ref))
        dt = abs(best[0] - t_ref)
        if dt > config.SYNC_WINDOW_SEC:
            return None, None, None
        return best[1], best[0], dt

    def trim_hist(self, cutoff):
        """측위 이력에서 이동 창 밖 항목 제거 (메모리 관리)."""
        self.range_hist = [tr for tr in self.range_hist if tr[0] >= cutoff]

    def trim_redraw(self, cutoff):
        """이동 창 밖 데이터 정리 후 곡선 갱신."""
        def slc(ts, *arrs):
            k = len(ts)
            for i, tt in enumerate(ts):
                if tt >= cutoff:
                    k = i
                    break
            return [a[k:] for a in (ts, *arrs)]

        self.times, self.ranges, self.ranges_f = slc(self.times, self.ranges, self.ranges_f)
        self.tx_times, self.tx_vals, self.tx_f = slc(self.tx_times, self.tx_vals, self.tx_f)
        self.out_t, self.out_r, self.out_x = slc(self.out_t, self.out_r, self.out_x)

        self.c_range_raw.setData(self.times, self.ranges)
        self.c_range_kf.setData(self.times, self.ranges_f)
        self.c_range_out.setData(self.out_t, self.out_r)
        self.c_tx_raw.setData(self.tx_times, self.tx_vals)
        self.c_tx_kf.setData(self.tx_times, self.tx_f)
        self.c_tx_out.setData(self.out_t, self.out_x)

    def last_time(self):
        """이 앵커가 가진 가장 최근 시각 (정상/아웃라이어 통틀어)."""
        t1 = self.times[-1] if self.times else None
        t2 = self.out_t[-1] if self.out_t else None
        if t1 is None:
            return t2
        if t2 is None:
            return t1
        return max(t1, t2)
