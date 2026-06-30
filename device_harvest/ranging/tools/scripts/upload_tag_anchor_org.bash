PLATFORMIO_BUILD_FLAGS="-DTAG_ID=0" pio run -e tag_dw1000_accuracy -t upload --upload-port /dev/ttyUSB0
PLATFORMIO_BUILD_FLAGS="-DANCHOR_ID=0" pio run -e anchor_dw1000_accuracy -t upload --upload-port /dev/ttyUSB1
PLATFORMIO_BUILD_FLAGS="-DANCHOR_ID=1" pio run -e anchor_dw1000_accuracy -t upload --upload-port /dev/ttyUSB2
PLATFORMIO_BUILD_FLAGS="-DANCHOR_ID=2" pio run -e anchor_dw1000_accuracy -t upload --upload-port /dev/ttyUSB3
