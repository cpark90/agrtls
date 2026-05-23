FROM osrf/ros:humble-desktop-full-jammy AS base

ENV DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-c"]

# =============================================================================
# SYSTEM
# =============================================================================

RUN apt update && apt install -y \
    build-essential \
    cmake \
    git \
    curl \
    wget \
    unzip \
    pkg-config \
    software-properties-common \
    python3-colcon-common-extensions \
    python3-rosdep \
    python3-vcstool \
    python3-pip \
    python3-dev \
    python3-empy \
    python3-numpy \
    nano \
    vim \
    zip \
    tar \
    rsync \
    ca-certificates \
    gnupg2 \
    lsb-release \
    libeigen3-dev \
    libboost-all-dev \
    libopencv-dev \
    libyaml-cpp-dev \
    libssl-dev \
    libatlas-base-dev \
    gfortran \
    libgoogle-glog-dev \
    libgflags-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libglew-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    mesa-common-dev \
    libglfw3-dev \
    libwayland-dev \
    wayland-protocols \
    libxkbcommon-dev \
    libudev-dev \
    libusb-1.0-0-dev \
    libjpeg-dev \
    zlib1g-dev \
    ffmpeg \
    && rm -rf /var/lib/apt/lists/*

# =============================================================================
# ROSDEP
# =============================================================================

RUN rosdep init || true
RUN rosdep update

# =============================================================================
# LIBREALSENSE
# =============================================================================

WORKDIR /root

RUN git clone https://github.com/IntelRealSense/librealsense.git && \
    cd librealsense && \
    git checkout v2.54.2 && \
    mkdir build && \
    cd build && \
    cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_EXAMPLES=false \
      -DBUILD_GRAPHICAL_EXAMPLES=false && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# =============================================================================
# PANGOLIN
# =============================================================================

WORKDIR /root

RUN git clone https://github.com/stevenlovegrove/Pangolin.git && \
    cd Pangolin && \
    git checkout v0.6 && \
    mkdir build && \
    cd build && \
    cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_PANGOLIN_GUI=OFF \
      -DBUILD_EXAMPLES=OFF \
      -DBUILD_TOOLS=OFF \
      -DBUILD_TESTS=OFF && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# =============================================================================
# WORKSPACE
# =============================================================================

WORKDIR /root/ros2_ws/src

# realsense ros
RUN git clone https://github.com/IntelRealSense/realsense-ros.git -b ros2-master

# ORB_SLAM3
RUN git clone https://github.com/UZ-SLAMLab/ORB_SLAM3.git

# ROS2 WRAPPER
RUN git clone https://github.com/zang09/ORB_SLAM3_ROS2.git

# =============================================================================
# PATCH ORB_SLAM3
# =============================================================================

WORKDIR /root/ros2_ws/src/ORB_SLAM3

# RUN git checkout c++14_comp

# RUN rm -rf Pangolin || true
# RUN rm -rf Thirdparty/Pangolin || true

# RUN sed -i '1i set(CMAKE_CXX_STANDARD 14)' CMakeLists.txt

# RUN sed -i '1i add_compile_options(-Wno-error)' CMakeLists.txt

# # disable viewer launch
# RUN sed -i '/mptViewer = new thread/d' src/System.cc || true
# RUN sed -i '/mptViewer->join/d' src/System.cc || true

# # =============================================================================
# # ENV
# # =============================================================================

# ENV Pangolin_DIR=/usr/local/lib/cmake/Pangolin

# ENV CMAKE_PREFIX_PATH=/usr/local:$CMAKE_PREFIX_PATH

# ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# # =============================================================================
# # BUILD ORB_SLAM3
# # =============================================================================

# WORKDIR /root/ros2_ws/src/ORB_SLAM3

# RUN mkdir build && \
#     cd build && \
#     cmake .. \
#       -DCMAKE_BUILD_TYPE=Release \
#       -DPangolin_DIR=/usr/local/lib/cmake/Pangolin && \
#     make -j$(nproc)

# # =============================================================================
# # BUILD ROS2
# # =============================================================================

# WORKDIR /root/ros2_ws

# RUN /bin/bash -c "\
# source /opt/ros/humble/setup.bash && \
# colcon build --symlink-install \
# "

# # =============================================================================
# # BASHRC
# # =============================================================================

# RUN echo "source /opt/ros/humble/setup.bash" >> /root/.bashrc

# RUN echo "source /root/ros2_ws/install/setup.bash" >> /root/.bashrc

# # =============================================================================
# # DEFAULT
# # =============================================================================

# WORKDIR /root/ros2_ws

# CMD ["/bin/bash"]
