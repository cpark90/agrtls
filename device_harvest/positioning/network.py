"""
네트워크 모듈.
논블로킹 UDP 소켓으로 수신하고, 줄 단위로 잘라 "RANGE,addr,range,rxpow"를 파싱합니다.
한 패킷에 여러 줄이 합쳐지거나 한 줄이 잘려 들어오는 경우를 버퍼로 처리합니다.
"""

import sys
import socket

import config


class UdpReceiver:
    def __init__(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.sock.bind((config.LISTEN_IP, config.LISTEN_PORT))
            self.sock.setblocking(False)
            print(f"Listening on {config.LISTEN_IP}:{config.LISTEN_PORT}")
        except Exception as e:
            print(f"UDP socket error: {e}")
            sys.exit(1)
        self.recv_buffer = ""

    @staticmethod
    def parse_line(line):
        """'RANGE,addr,range,rxpow' → (dev_id 대문자 HEX, range, rx) 또는 (None, None, None)."""
        try:
            parts = line.split(',')
            if len(parts) >= 4 and parts[0].strip() == "RANGE":
                return parts[1].strip().upper(), float(parts[2]), float(parts[3])
        except ValueError:
            pass
        return None, None, None

    def poll(self):
        """버퍼에 쌓인 모든 패킷을 비우고, 파싱된 (dev_id, range, rx) 리스트를 반환."""
        results = []
        while True:
            try:
                data, _ = self.sock.recvfrom(2048)
            except BlockingIOError:
                break
            except Exception as e:
                print(f"UDP read error: {e}")
                break
            self.recv_buffer += data.decode('utf-8', errors='ignore')
            while '\n' in self.recv_buffer:
                line, self.recv_buffer = self.recv_buffer.split('\n', 1)
                line = line.strip()
                if not line:
                    continue
                dev_id, rng, rx = self.parse_line(line)
                if dev_id is not None:
                    results.append((dev_id, rng, rx))
        return results

    def close(self):
        self.sock.close()
        print("UDP socket closed.")
