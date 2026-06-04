import serial
import struct
import time
import signal
import sys

OTA_REQ    = 1
OTA_READY  = 2
OTA_DATA   = 3
OTA_ACK    = 4
OTA_END    = 5
OTA_NOTIFY = 6
OTA_CANCEL = 7

OTA_CHUNK = 1024

ser = serial.Serial("/dev/ttyUSB0",115200)

fw = open("firmware.bin","rb").read()
size = len(fw)

def send_cancel():
    pkt = struct.pack("<B", OTA_CANCEL)
    ser.write(pkt)
    print("cancel sent")

def handler(sig,frame):
    send_cancel()
    ser.close()
    sys.exit(0)

signal.signal(signal.SIGINT, handler)

print("serial connected")

# notify
notify = struct.pack("<BH", OTA_NOTIFY,1)
ser.write(notify)

print("notify sent")

# wait req
data = ser.read(3)

t,ver = struct.unpack("<BH",data)

if t != OTA_REQ:
    print("invalid req")
    send_cancel()
    sys.exit()

print("req received")

# ready
ready = struct.pack("<B", OTA_READY)
ser.write(ready)

print("ready sent")

seq = 0
offset = 0

while offset < size:

    chunk = fw[offset:offset+OTA_CHUNK]
    length = len(chunk)

    packet = struct.pack("<BHH",OTA_DATA,seq,length) + chunk

    ser.write(packet)

    ack = ser.read(3)

    t,ackseq = struct.unpack("<BH",ack)

    if t != OTA_ACK or ackseq != seq:
        print("ack mismatch")
        send_cancel()
        sys.exit()

    offset += length
    seq += 1

    print("sent",offset)

    time.sleep(0.003)

end = struct.pack("<BHH",OTA_END,0,0)

ser.write(end)

print("OTA finished")

ser.close()
