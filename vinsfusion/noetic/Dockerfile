FROM osrf/ros:noetic-desktop-full

ENV DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-c"]

# -----------------------------------------------------------------------------
# Basic packages
# -----------------------------------------------------------------------------

RUN apt-get update && apt-get install -y \
    git \
    wget \
    curl \
    vim \
    build-essential \
    cmake \
    pkg-config \
    unzip \
    software-properties-common \
    python3-pip \
    python3-rosdep \
    python3-catkin-tools \
    python3-osrf-pycommon \
    libeigen3-dev \
    libopencv-dev \
    libgoogle-glog-dev \
    libgflags-dev \
    libsuitesparse-dev \
    libatlas-base-dev \
    libceres-dev \
    libyaml-cpp-dev \
    libdw-dev \
    libusb-1.0-0-dev \
    libgtk-3-dev \
    libglfw3-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    ros-noetic-cv-bridge \
    ros-noetic-image-transport \
    ros-noetic-tf \
    ros-noetic-rviz \
    ros-noetic-ddynamic-reconfigure \
    ros-noetic-camera-info-manager \
    ros-noetic-diagnostic-updater \
    && rm -rf /var/lib/apt/lists/*

# -----------------------------------------------------------------------------
# ROSDEP
# -----------------------------------------------------------------------------

RUN rosdep init || true
RUN rosdep update

# -----------------------------------------------------------------------------
# Catkin workspace
# -----------------------------------------------------------------------------

RUN mkdir -p /root/catkin_ws/src

WORKDIR /root/catkin_ws/src

# -----------------------------------------------------------------------------
# librealsense
# -----------------------------------------------------------------------------
WORKDIR /root

RUN git clone https://github.com/IntelRealSense/librealsense.git

WORKDIR /root/librealsense

RUN git checkout v2.56.5

RUN mkdir build

WORKDIR /root/librealsense/build

RUN cmake .. \
    -DBUILD_EXAMPLES=false \
    -DBUILD_GRAPHICAL_EXAMPLES=false \
    -DBUILD_WITH_CUDA=false \
    -DFORCE_RSUSB_BACKEND=true

RUN make -j$(nproc)

RUN make install

# -----------------------------------------------------------------------------
# realsense-ros
# -----------------------------------------------------------------------------

WORKDIR /root/catkin_ws/src

RUN git clone https://github.com/IntelRealSense/realsense-ros.git

WORKDIR /root/catkin_ws/src/realsense-ros

RUN git checkout ros1-legacy

# -----------------------------------------------------------------------------
# VINS-Fusion
# -----------------------------------------------------------------------------

WORKDIR /root/catkin_ws/src

RUN git clone https://github.com/rkuo2000/VINS-Fusion.git

# -----------------------------------------------------------------------------
# Build
# -----------------------------------------------------------------------------

WORKDIR /root/catkin_ws

RUN source /opt/ros/noetic/setup.bash && \
    rosdep install --from-paths src \
    --ignore-src \
    -r -y \
    --skip-keys="libudev-dev dkms udev"

RUN source /opt/ros/noetic/setup.bash && \
    catkin_make -DCMAKE_BUILD_TYPE=Release

# -----------------------------------------------------------------------------
# Environment
# -----------------------------------------------------------------------------

RUN echo "source /opt/ros/noetic/setup.bash" >> /root/.bashrc
RUN echo "source /root/catkin_ws/devel/setup.bash" >> /root/.bashrc

WORKDIR /root/catkin_ws

CMD ["/bin/bash"]
