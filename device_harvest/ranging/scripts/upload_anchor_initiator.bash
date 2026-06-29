PLATFORMIO_BUILD_FLAGS="-DANCHOR_ID=0" pio run -e anchor_dw1000_accuracy_meshagent -t upload --upload-port /dev/ttyUSB1
PLATFORMIO_BUILD_FLAGS="-DANCHOR_ID=1" pio run -e anchor_dw1000_accuracy_meshagent -t upload --upload-port /dev/ttyUSB2
