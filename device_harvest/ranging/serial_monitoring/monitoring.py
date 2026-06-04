import sys
import time
import re
import serial
import pyqtgraph as pg
from PyQt5.QtCore import QTimer
from PyQt5.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget

# ==================== 사용자 설정 ====================
SERIAL_PORT = "/dev/ttyUSB0"       # 본인의 ESP32 연결 포트로 변경 (예: Linux는 '/dev/ttyUSB0')
BAUD_RATE = 115200         # 시리얼 통신 속도
WINDOW_TIME_SEC = 10       # 이동 창(Moving Window) 시간 범위 (10초)
# ====================================================

class UwbVisualizer(QMainWindow):
	def __init__(self):
		super().__init__()
		self.setWindowTitle("UWB Real-time Moving Window Visualizer")
		self.resize(800, 600)

		# 시리얼 연결 시도
		try:
			self.ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
			print(f"Connected to {SERIAL_PORT}")
		except Exception as e:
			print(f"Serial connection error: {e}")
			sys.exit(1)

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

	def parse_serial_line(self, line):
		# 정수 및 소수점 형태의 숫자 매칭 정규식

		try:
			parts = line.split(',')
			if len(parts) >= 4 and parts[0].strip() == "RANGE":
				# 첫 번째 숫자를 Range, 두 번째 숫자를 TX 값으로 인식
				val_range = float(parts[2].strip())
				val_tx = float(parts[3].strip())
			return val_range, val_tx
		except ValueError:
			return None, None
		return None, None

	def update_data(self):
		# 시리얼 버퍼에 읽을 데이터가 있는지 확인
		while self.ser.in_waiting > 0:
			try:
				line_bytes = self.ser.readline()
				line = line_bytes.decode('utf-8', errors='ignore').strip()

				if not line:
					continue

				# 데이터 파싱
				val_range, val_tx = self.parse_serial_line(line)

				if val_range is not None and val_tx is not None:
					current_time = time.time() - self.start_time

					# 데이터 배열에 추가
					self.times.append(current_time)
					self.ranges.append(val_range)
					self.tx_values.append(val_tx)

			except Exception as e:
				print(f"Error reading serial: {e}")

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
		# 프로그램 종료 시 시리얼 포트 닫기
		if self.ser.is_open:
			self.ser.close()
			print("Serial port closed.")
		event.accept()

if __name__ == "__main__":
	app = QApplication(sys.argv)
	visualizer = UwbVisualizer()
	visualizer.show()
	sys.exit(app.exec_())
