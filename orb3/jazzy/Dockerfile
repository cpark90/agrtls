FROM osrf/ros:jazzy-desktop

SHELL ["/bin/bash", "-c"]

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    git \
    cmake \
    python3-pip \
    libgoogle-glog-dev \
    libatlas-base-dev \
    libsuitesparse-dev \
    libglew-dev \
    libgtk-3-dev \
    libusb-1.0-0-dev \
    pkg-config \
    libssl-dev \
    libboost-all-dev \
    libopencv-dev \
    libeigen3-dev \
    ros-jazzy-vision-opencv \
    ros-jazzy-message-filters \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt
RUN git clone https://github.com/IntelRealSense/librealsense.git && \
    cd librealsense && \
    git checkout v2.56.3 && \
    mkdir build && \
    cd build && \
    cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_EXAMPLES=false \
      -DBUILD_GRAPHICAL_EXAMPLES=false \
      -DCMAKE_C_FLAGS="-I/usr/include/libusb-1.0" \
      -DCMAKE_CXX_FLAGS="-I/usr/include/libusb-1.0" && \
    make -j$(nproc) && \
    make install && \
    ldconfig

WORKDIR /root/ros2_ws/src
RUN git clone https://github.com/IntelRealSense/realsense-ros.git -b ros2-development

WORKDIR /root/ros2_ws
RUN apt-get update && \
    rosdep init || true && \
    rosdep update && \
    rosdep install --from-paths src --ignore-src -r -y && \
    source /opt/ros/jazzy/setup.bash && \
    colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release

WORKDIR /opt
RUN git clone --recursive https://github.com/stevenlovegrove/Pangolin.git
RUN cd Pangolin && ./scripts/install_prerequisites.sh --dry-run recommended && \
    ./scripts/install_prerequisites.sh -m apt all


WORKDIR /root/ros2_ws/src
RUN git clone https://github.com/zang09/ORB_SLAM3_ROS2.git orbslam3_ros2

