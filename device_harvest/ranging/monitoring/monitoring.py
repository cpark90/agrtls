import sys
import time
import socket
import math
import pyqtgraph as pg
from PyQt5.QtCore import QTimer
from PyQt5.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget

# ==================== 사용자 설정 ====================
LISTEN_IP = "0.0.0.0"      # 모든 인터페이스에서 수신
LISTEN_PORT = 5005         # ESP32의 SERVER_PORT와 동일하게
WINDOW_TIME_SEC = 10       # 이동 창(Moving Window) 시간 범위 (10초)

# 칼만 필터 튜닝 파라미터 (Range 용)
RANGE_Q = 0.01
RANGE_R = 0.1

# 칼만 필터 튜닝 파라미터 (RX Power 용)
RX_Q = 0.05
RX_R = 4.0

# 아웃라이어 리젝션 파라미터
GATE_SIGMA = 3.0
MAX_REJECT_STREAK = 5

# 디바이스(id)별로 순환 사용할 색상 팔레트
COLOR_PALETTE = [
	(0, 200, 0),     # green
	(255, 60, 60),   # red
	(60, 120, 255),  # blue
	(255, 180, 0),   # orange
	(200, 0, 200),   # magenta
	(0, 200, 200),   # cyan
	(180, 180, 0),   # olive
	(255, 120, 180), # pink
]
# ====================================================


class KalmanFilter1D:
	"""1차원(스칼라) 칼만 필터. 검사/갱신 분리."""
	def __init__(self, q=0.01, r=0.1, gate_sigma=3.0):
		self.q = q
		self.r = r
		self.gate_sigma = gate_sigma
		self.x = None
		self.p = 1.0
		self._p_pred = None
		self._innov = None

	def check_outlier(self, measurement):
		if self.x is None:
			self._p_pred = self.p + self.q
			self._innov = 0.0
			return False
		p_pred = self.p + self.q
		innovation = measurement - self.x
		s = p_pred + self.r
		gate = self.gate_sigma * math.sqrt(s)
		self._p_pred = p_pred
		self._innov = innovation
		return abs(innovation) > gate

	def accept(self, measurement):
		if self.x is None:
			self.x = measurement
			self.p = 1.0
			return self.x
		p_pred = self._p_pred
		s = p_pred + self.r
		k = p_pred / s
		self.x += k * self._innov
		self.p = (1 - k) * p_pred
		return self.x

	def reject(self):
		if self._p_pred is not None:
			self.p = self._p_pred
		return self.x


class DeviceTrack:
	"""디바이스(short address) 하나에 대한 상태 묶음.
	필터 2개 + 데이터 버퍼 + 그래프 곡선들."""
	def __init__(self, dev_id, color, p1, p2):
		self.dev_id = dev_id
		self.color = color

		# 두 채널 필터
		self.kf_range = KalmanFilter1D(q=RANGE_Q, r=RANGE_R, gate_sigma=GATE_SIGMA)
		self.kf_rx = KalmanFilter1D(q=RX_Q, r=RX_R, gate_sigma=GATE_SIGMA)
		self.reject_streak = 0

		# 데이터 버퍼
		self.times = []
		self.ranges = []
		self.ranges_filtered = []
		self.tx_values = []
		self.tx_filtered = []
		# 아웃라이어(표시용)
		self.outlier_times = []
		self.outlier_ranges = []
		self.outlier_tx = []

		# ----- 그래프 곡선 생성 -----
		raw_pen = pg.mkPen((color[0], color[1], color[2], 90), width=1)  # 반투명 원본
		kf_pen = pg.mkPen(color, width=2)                                # 필터값

		# Range 그래프
		self.curve_range_raw = p1.plot(pen=raw_pen)
		self.curve_range_kf = p1.plot(pen=kf_pen, name=f"{dev_id} range")
		self.curve_range_outlier = p1.plot(
			pen=None, symbol='x', symbolPen=pg.mkPen(color, width=2),
			symbolBrush=color, symbolSize=10)

		# RX 그래프
		self.curve_tx_raw = p2.plot(pen=raw_pen)
		self.curve_tx_kf = p2.plot(pen=kf_pen, name=f"{dev_id} rx")
		self.curve_tx_outlier = p2.plot(
			pen=None, symbol='x', symbolPen=pg.mkPen(color, width=2),
			symbolBrush=color, symbolSize=10)

	def process_sample(self, val_range, val_tx, t):
		# 두 채널 아웃라이어 검사
		out_range = self.kf_range.check_outlier(val_range)
		out_rx = self.kf_rx.check_outlier(val_tx)
		is_outlier = out_range or out_rx

		# 거부 결정 (연속 거부 한계 넘으면 강제 수용)
		if is_outlier and self.reject_streak < MAX_REJECT_STREAK:
			self.kf_range.reject()
			self.kf_rx.reject()
			self.reject_streak += 1
			self.outlier_times.append(t)
			self.outlier_ranges.append(val_range)
			self.outlier_tx.append(val_tx)
			return

		# 정상 샘플 → 둘 다 갱신
		self.reject_streak = 0
		r_kf = self.kf_range.accept(val_range)
		x_kf = self.kf_rx.accept(val_tx)
		self.times.append(t)
		self.ranges.append(val_range)
		self.ranges_filtered.append(r_kf)
		self.tx_values.append(val_tx)
		self.tx_filtered.append(x_kf)

	def trim_and_redraw(self, cutoff_time):
		# 정상 데이터 이동 창 슬라이싱
		if self.times:
			keep_idx = 0
			for i, t in enumerate(self.times):
				if t >= cutoff_time:
					keep_idx = i
					break
			self.times = self.times[keep_idx:]
			self.ranges = self.ranges[keep_idx:]
			self.ranges_filtered = self.ranges_filtered[keep_idx:]
			self.tx_values = self.tx_values[keep_idx:]
			self.tx_filtered = self.tx_filtered[keep_idx:]

		# 아웃라이어 이동 창 정리
		t, r, x = [], [], []
		for ti, ri, xi in zip(self.outlier_times, self.outlier_ranges, self.outlier_tx):
			if ti >= cutoff_time:
				t.append(ti); r.append(ri); x.append(xi)
		self.outlier_times, self.outlier_ranges, self.outlier_tx = t, r, x

		# 곡선 갱신
		self.curve_range_raw.setData(self.times, self.ranges)
		self.curve_range_kf.setData(self.times, self.ranges_filtered)
		self.curve_range_outlier.setData(self.outlier_times, self.outlier_ranges)
		self.curve_tx_raw.setData(self.times, self.tx_values)
		self.curve_tx_kf.setData(self.times, self.tx_filtered)
		self.curve_tx_outlier.setData(self.outlier_times, self.outlier_tx)

	def last_time(self):
		# 이 디바이스가 가진 가장 최근 시각 (정상/아웃라이어 통틀어)
		t1 = self.times[-1] if self.times else None
		t2 = self.outlier_times[-1] if self.outlier_times else None
		if t1 is None:
			return t2
		if t2 is None:
			return t1
		return max(t1, t2)


class UwbVisualizer(QMainWindow):
	def __init__(self):
		super().__init__()
		self.setWindowTitle("UWB Visualizer (Per-Device: Kalman + Outlier Rejection)")
		self.resize(900, 650)

		# UDP 소켓
		try:
			self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
			self.sock.bind((LISTEN_IP, LISTEN_PORT))
			self.sock.setblocking(False)
			print(f"Listening on {LISTEN_IP}:{LISTEN_PORT}")
		except Exception as e:
			print(f"UDP socket error: {e}")
			sys.exit(1)

		self.recv_buffer = ""
		self.start_time = time.time()

		# 디바이스별 트랙 저장: { dev_id: DeviceTrack }
		self.tracks = {}
		self.color_idx = 0

		# UI
		central_widget = QWidget()
		self.setCentralWidget(central_widget)
		layout = QVBoxLayout(central_widget)
		self.win = pg.GraphicsLayoutWidget()
		layout.addWidget(self.win)

		# Range 그래프
		self.p1 = self.win.addPlot(title="UWB Range (per device) - Raw vs Kalman")
		self.p1.setLabel('left', 'Range', units='m')
		self.p1.setLabel('bottom', 'Time', units='s')
		self.p1.showGrid(x=True, y=True)
		self.p1.addLegend()

		self.win.nextRow()

		# RX 그래프
		self.p2 = self.win.addPlot(title="UWB RX Power (per device) - Raw vs Kalman")
		self.p2.setLabel('left', 'RX Power')
		self.p2.setLabel('bottom', 'Time', units='s')
		self.p2.showGrid(x=True, y=True)
		self.p2.addLegend()
		self.p2.setXLink(self.p1)

		# 타이머
		self.timer = QTimer()
		self.timer.timeout.connect(self.update_data)
		self.timer.start(20)

	def parse_line(self, line):
		# "RANGE,addr,range,rxpow" → (addr, range, rx)
		try:
			parts = line.split(',')
			if len(parts) >= 4 and parts[0].strip() == "RANGE":
				dev_id = parts[1].strip()           # short address (앵커/태그 구분 키)
				val_range = float(parts[2].strip())
				val_tx = float(parts[3].strip())
				return dev_id, val_range, val_tx
		except ValueError:
			return None, None, None
		return None, None, None

	def get_track(self, dev_id):
		# 처음 보는 id면 새 트랙(필터+곡선) 생성
		if dev_id not in self.tracks:
			color = COLOR_PALETTE[self.color_idx % len(COLOR_PALETTE)]
			self.color_idx += 1
			self.tracks[dev_id] = DeviceTrack(dev_id, color, self.p1, self.p2)
			print(f"New device track: {dev_id}  color={color}")
		return self.tracks[dev_id]

	def update_data(self):
		# UDP 패킷 모두 비우기
		while True:
			try:
				data, _ = self.sock.recvfrom(2048)
			except BlockingIOError:
				break
			except Exception as e:
				print(f"Error reading UDP: {e}")
				break

			self.recv_buffer += data.decode('utf-8', errors='ignore')

			while '\n' in self.recv_buffer:
				line, self.recv_buffer = self.recv_buffer.split('\n', 1)
				line = line.strip()
				if not line:
					continue

				dev_id, val_range, val_tx = self.parse_line(line)
				if dev_id is not None:
					t = time.time() - self.start_time
					self.get_track(dev_id).process_sample(val_range, val_tx, t)

		# 전체 디바이스 중 가장 최근 시각 기준으로 이동 창 설정
		latest = None
		for tr in self.tracks.values():
			lt = tr.last_time()
			if lt is not None and (latest is None or lt > latest):
				latest = lt

		if latest is not None:
			cutoff_time = latest - WINDOW_TIME_SEC
			for tr in self.tracks.values():
				tr.trim_and_redraw(cutoff_time)
			self.p1.setXRange(cutoff_time, latest, padding=0)

	def closeEvent(self, event):
		self.sock.close()
		print("UDP socket closed.")
		event.accept()


if __name__ == "__main__":
	app = QApplication(sys.argv)
	visualizer = UwbVisualizer()
	visualizer.show()
	sys.exit(app.exec_())
