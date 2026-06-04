docker run -it --rm \
    --net=host \
    --privileged \
    -v /dev:/dev \
    -v $(pwd):/data \
    kalibr-noetic
