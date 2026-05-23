docker run -it \
  --privileged \
  --network host \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v /dev:/dev \
  vins-noetic:20.04
