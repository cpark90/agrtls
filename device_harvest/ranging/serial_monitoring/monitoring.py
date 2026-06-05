import sys
import time
import socket
import pyqtgraph as pg
from PyQt5.QtCore import QTimer
from PyQt5.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget

# ==================== 사용자 설정 ====================
LISTEN_IP = "0.0.0.0"      # 모든 인터페이스에서 수신 (보통 그대로 둠)
LISTEN_PORT = 5005         # ESP32의 SERVER_PORT와 동일하게
WINDOW_TIME_SEC = 10       # 이동 창(Moving Window) 시간 범위 (10초)
# ====================================================

class UwbVisualizer(QMainWindow):
	def __init__(self):
		super().__init__()
		self.setWindowTitle("UWB Real-time Moving Window Visualizer (UDP)")
		self.resize(800, 600)

		# UDP 소켓 연결 시도
		try:
			self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
			self.sock.bind((LISTEN_IP, LISTEN_PORT))
			self.sock.setblocking(False)   # 논블로킹 모드 (버퍼 비어있으면 즉시 반환)
			print(f"Listening on {LISTEN_IP}:{LISTEN_PORT}")
		except Exception as e:
			print(f"UDP socket error: {e}")
			sys.exit(1)

		# 수신 버퍼 (여러 줄이 한 패킷에 합쳐 들어올 경우 대비)
		self.recv_buffer = ""

		# 데이터 저장용 리스트 (시간, 거리, TX)
		self.times = []
		self.ranges = []
		self.tx_values = []
		self.start_time = time.time()

		# UI 및 그래프 레이아웃 설정
		central_widget = QWidget()
		self.setCentralWidget(central_widget)
		layout = QVBoxLayout(central_widget)

		# pyqtgraph 윈도우 생성
		self.win = pg.GraphicsLayoutWidget()
		layout.addWidget(self.win)

		# 첫 번째 그래프: Range (거리)
		self.p1 = self.win.addPlot(title="UWB Range (Distance)")
		self.p1.setLabel('left', 'Range', units='m')
		self.p1.setLabel('bottom', 'Time', units='s')
		self.p1.showGrid(x=True, y=True)
		self.curve_range = self.p1.plot(pen=pg.mkPen('g', width=2)) # 녹색 선

		self.win.nextRow() # 다음 줄에 그래프 배치

		# 두 번째 그래프: TX 값
		self.p2 = self.win.addPlot(title="UWB TX Value / Power")
		self.p2.setLabel('left', 'TX Value')
		self.p2.setLabel('bottom', 'Time', units='s')
		self.p2.showGrid(x=True, y=True)
		self.curve_tx = self.p2.plot(pen=pg.mkPen('r', width=2)) # 적색 선

		# 두 그래프의 X축(시간축) 동기화
		self.p2.setXLink(self.p1)

		# 실시간 업데이트를 위한 타이머 설정 (20ms 마다 주기적 실행)
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
				break   # 더 이상 받을 데이터 없음
			except Exception as e:
				print(f"Error reading UDP: {e}")
				break

			self.recv_buffer += data.decode('utf-8', errors='ignore')

			# 줄바꿈 기준으로 완성된 줄만 처리
			while '\n' in self.recv_buffer:
				line, self.recv_buffer = self.recv_buffer.split('\n', 1)
				line = line.strip()
				if not line:
					continue

				val_range, val_tx = self.parse_line(line)
				if val_range is not None and val_tx is not None:
					current_time = time.time() - self.start_time
					self.times.append(current_time)
					self.ranges.append(val_range)
					self.tx_values.append(val_tx)

		# 이동 창(Moving Window) 처리: 현재 시간 기준 WINDOW_TIME_SEC 이전 데이터는 버림
		if self.times:
			now = self.times[-1]
			cutoff_time = now - WINDOW_TIME_SEC

			# 오래된 데이터의 인덱스 찾기
			keep_idx = 0
			for i, t in enumerate(self.times):
				if t >= cutoff_time:
					keep_idx = i
					break

			# 윈도우 크기에 맞게 슬라이싱
			self.times = self.times[keep_idx:]
			self.ranges = self.ranges[keep_idx:]
			self.tx_values = self.tx_values[keep_idx:]

			# 그래프 데이터 갱신
			self.curve_range.setData(self.times, self.ranges)
			self.curve_tx.setData(self.times, self.tx_values)

			# X축 범위를 최근 N초 영역으로 고정 (이동 창 연출)
			self.p1.setXRange(cutoff_time, now, padding=0)

	def closeEvent(self, event):
		# 프로그램 종료 시 소켓 닫기
		self.sock.close()
		print("UDP socket closed.")
		event.accept()

if __name__ == "__main__":
	app = QApplication(sys.argv)
	visualizer = UwbVisualizer()
	visualizer.show()
	sys.exit(app.exec_())
