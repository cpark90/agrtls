import sys
import time
import socket
import math
import pyqtgraph as pg
from PyQt5.QtCore import QTimer
from PyQt5.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget

# ==================== 사용자 설정 ====================
LISTEN_IP = "0.0.0.0"      # 모든 인터페이스에서 수신 (보통 그대로 둠)
LISTEN_PORT = 5005         # ESP32의 SERVER_PORT와 동일하게
WINDOW_TIME_SEC = 10       # 이동 창(Moving Window) 시간 범위 (10초)

# 칼만 필터 튜닝 파라미터
KALMAN_Q = 0.01    # 프로세스 잡음: 클수록 측정값을 빨리 따라감(반응↑, 노이즈↑)
KALMAN_R = 0.1     # 측정 잡음: 클수록 측정값을 덜 믿음(부드러움↑, 지연↑)

# 아웃라이어 리젝션 파라미터
GATE_SIGMA = 3.0       # 게이팅 임계값(시그마). 예측 분포에서 이 배수 이상 벗어나면 아웃라이어로 간주
MAX_REJECT_STREAK = 5  # 연속 거부가 이 횟수를 넘으면 실제 변화로 보고 필터를 재수렴(강제 수용)
# ====================================================


class KalmanFilter1D:
	"""1차원(스칼라) 칼만 필터 - 거리값 평활화 + 아웃라이어 리젝션"""
	def __init__(self, q=0.01, r=0.1, gate_sigma=3.0, max_reject_streak=5):
		self.q = q                  # 프로세스 잡음 분산
		self.r = r                  # 측정 잡음 분산
		self.gate_sigma = gate_sigma
		self.max_reject_streak = max_reject_streak

		self.x = None               # 추정값 (상태)
		self.p = 1.0                # 추정 오차 공분산
		self.reject_streak = 0      # 연속 거부 횟수

	def update(self, measurement):
		# 첫 측정값으로 초기화
		if self.x is None:
			self.x = measurement
			return self.x, False  # (추정값, 아웃라이어 여부)

		# 예측 단계 (상태 변화 없다고 가정, 오차만 증가)
		p_pred = self.p + self.q

		# 혁신(innovation)과 혁신 공분산
		innovation = measurement - self.x
		s = p_pred + self.r                 # 혁신 분산
		gate = self.gate_sigma * math.sqrt(s)  # 허용 범위(±gate)

		# 아웃라이어 게이팅 검사
		is_outlier = abs(innovation) > gate

		if is_outlier and self.reject_streak < self.max_reject_streak:
			# 아웃라이어로 판단 → 갱신하지 않고 예측만 유지
			self.reject_streak += 1
			self.p = p_pred  # 거부해도 불확실성은 누적시켜 둠(다음 게이트가 넓어짐)
			return self.x, True

		# 정상 측정(또는 연속 거부 한계 초과 → 강제 수용)으로 갱신
		self.reject_streak = 0
		k = p_pred / s                      # 칼만 이득
		self.x += k * innovation
		self.p = (1 - k) * p_pred
		return self.x, False


class UwbVisualizer(QMainWindow):
	def __init__(self):
		super().__init__()
		self.setWindowTitle("UWB Real-time Moving Window Visualizer (UDP + Kalman + Outlier Rejection)")
		self.resize(800, 600)

		# UDP 소켓 연결 시도
		try:
			self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
			self.sock.bind((LISTEN_IP, LISTEN_PORT))
			self.sock.setblocking(False)   # 논블로킹 모드
			print(f"Listening on {LISTEN_IP}:{LISTEN_PORT}")
		except Exception as e:
			print(f"UDP socket error: {e}")
			sys.exit(1)

		# 수신 버퍼
		self.recv_buffer = ""

		# 칼만 필터 인스턴스
		self.kf = KalmanFilter1D(
			q=KALMAN_Q, r=KALMAN_R,
			gate_sigma=GATE_SIGMA, max_reject_streak=MAX_REJECT_STREAK)

		# 데이터 저장용 리스트
		self.times = []
		self.ranges = []            # 원본(정상 측정)
		self.ranges_filtered = []   # 칼만 결과
		self.tx_values = []
		# 아웃라이어로 거부된 점(따로 표시용): 시간, 값
		self.outlier_times = []
		self.outlier_values = []
		self.start_time = time.time()

		# UI 및 그래프 레이아웃 설정
		central_widget = QWidget()
		self.setCentralWidget(central_widget)
		layout = QVBoxLayout(central_widget)

		self.win = pg.GraphicsLayoutWidget()
		layout.addWidget(self.win)

		# 첫 번째 그래프: Range (거리)
		self.p1 = self.win.addPlot(title="UWB Range (Distance) - Raw vs Kalman")
		self.p1.setLabel('left', 'Range', units='m')
		self.p1.setLabel('bottom', 'Time', units='s')
		self.p1.showGrid(x=True, y=True)
		self.p1.addLegend()
		# 원본: 흐린 회색 얇은 선
		self.curve_range_raw = self.p1.plot(
			pen=pg.mkPen((150, 150, 150), width=1), name="Raw")
		# 필터링: 굵은 녹색 선
		self.curve_range_kf = self.p1.plot(
			pen=pg.mkPen('g', width=2), name="Kalman")
		# 아웃라이어: 빨간 X 마커 (선 없음)
		self.curve_outlier = self.p1.plot(
			pen=None, symbol='x', symbolPen=pg.mkPen('r', width=2),
			symbolBrush='r', symbolSize=10, name="Outlier")

		self.win.nextRow()

		# 두 번째 그래프: TX 값
		self.p2 = self.win.addPlot(title="UWB TX Value / Power")
		self.p2.setLabel('left', 'TX Value')
		self.p2.setLabel('bottom', 'Time', units='s')
		self.p2.showGrid(x=True, y=True)
		self.curve_tx = self.p2.plot(pen=pg.mkPen('r', width=2))

		# X축 동기화
		self.p2.setXLink(self.p1)

		# 실시간 업데이트 타이머
		self.timer = QTimer()
		self.timer.timeout.connect(self.update_data)
		self.timer.start(20)

	def parse_line(self, line):
		# "RANGE,addr,range,rxpow" 형태 파싱
		try:
			parts = line.split(',')
			if len(parts) >= 4 and parts[0].strip() == "RANGE":
				val_range = float(parts[2].strip())
				val_tx = float(parts[3].strip())
				return val_range, val_tx
		except ValueError:
			return None, None
		return None, None

	def update_data(self):
		# UDP 소켓에서 받을 수 있는 모든 패킷을 비움
		while True:
			try:
				data, _ = self.sock.recvfrom(1024)
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

				val_range, val_tx = self.parse_line(line)
				if val_range is not None and val_tx is not None:
					current_time = time.time() - self.start_time

					# 칼만 필터 + 아웃라이어 검사
					val_range_kf, is_outlier = self.kf.update(val_range)

					if is_outlier:
						# 거부된 측정값은 아웃라이어 마커로만 기록
						self.outlier_times.append(current_time)
						self.outlier_values.append(val_range)
					else:
						# 정상 측정값만 원본/필터 라인에 기록
						self.times.append(current_time)
						self.ranges.append(val_range)
						self.ranges_filtered.append(val_range_kf)
						self.tx_values.append(val_tx)

		# 이동 창 처리
		if self.times:
			now = self.times[-1]
			cutoff_time = now - WINDOW_TIME_SEC

			keep_idx = 0
			for i, t in enumerate(self.times):
				if t >= cutoff_time:
					keep_idx = i
					break

			self.times = self.times[keep_idx:]
			self.ranges = self.ranges[keep_idx:]
			self.ranges_filtered = self.ranges_filtered[keep_idx:]
			self.tx_values = self.tx_values[keep_idx:]

			# 아웃라이어도 같은 창 기준으로 정리
			self.outlier_times, self.outlier_values = self._trim_outliers(cutoff_time)

			# 그래프 갱신
			self.curve_range_raw.setData(self.times, self.ranges)
			self.curve_range_kf.setData(self.times, self.ranges_filtered)
			self.curve_outlier.setData(self.outlier_times, self.outlier_values)
			self.curve_tx.setData(self.times, self.tx_values)

			self.p1.setXRange(cutoff_time, now, padding=0)

	def _trim_outliers(self, cutoff_time):
		# 이동 창 밖으로 나간 아웃라이어 점 제거
		new_t, new_v = [], []
		for t, v in zip(self.outlier_times, self.outlier_values):
			if t >= cutoff_time:
				new_t.append(t)
				new_v.append(v)
		return new_t, new_v

	def closeEvent(self, event):
		self.sock.close()
		print("UDP socket closed.")
		event.accept()


if __name__ == "__main__":
	app = QApplication(sys.argv)
	visualizer = UwbVisualizer()
	visualizer.show()
	sys.exit(app.exec_())
