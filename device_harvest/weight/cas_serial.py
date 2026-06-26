import serial
import time

# ── 설정 ─────────────────────────────────
PORT = "/dev/ttyUSB0"   # Windows는 "COM3" 등
BAUDRATE = 9600          # 장비 매뉴얼에서 확인 필요
TIMEOUT = 1

def main():
    ser = serial.Serial(
        port=PORT,
        baudrate=BAUDRATE,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=TIMEOUT,
    )

    print(f"연결됨: {ser.name}")

    try:
        while True:
            if ser.in_waiting > 0:
                raw = ser.readline()
                try:
                    decoded = raw.decode("utf-8", errors="replace").strip()
                except Exception as e:
                    decoded = f"디코딩 오류: {e}"

                timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
                print(f"[{timestamp}] {decoded}")

    except KeyboardInterrupt:
        print("\n종료합니다.")
    finally:
        ser.close()

if __name__ == "__main__":
    main()
