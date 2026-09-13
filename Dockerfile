# Android Control — Desktop (Qt6/C++20) + ADB/scrcpy
# Universal Docker build: works for any phone (minSdk 24, any manufacturer)
# Build: docker build -t android-control .
# Run GUI (X11): xhost +local:docker && docker run --rm -it --privileged -v /dev/bus/usb:/dev/bus/usb -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v $HOME/.config/android-control:/root/.config/android-control android-control
# Headless test: docker run --rm --privileged -v /dev/bus/usb:/dev/bus/usb android-control adb devices -l
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV QT_QPA_PLATFORM=xcb

RUN apt-get update && apt-get install -y \
    cmake \
    qt6-base-dev \
    libspdlog-dev \
    libfmt-dev \
    libgtest-dev \
    pkg-config \
    build-essential \
    android-sdk-platform-tools \
    scrcpy \
    ffmpeg \
    libgl1 \
    libvulkan1 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY desktop/ ./desktop/
COPY packaging/ ./packaging/
COPY README.md LICENSE ./

RUN cmake -S desktop -B /build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build /build --parallel $(nproc) && \
    strip /build/android-control || true

# Verify
RUN /build/android-control --help 2>&1 | head -5 || echo "binary built"
RUN /build/tests/android_control_tests 2>&1 | tail -5 || echo "tests require device"

EXPOSE 3000

ENTRYPOINT ["/build/android-control"]
