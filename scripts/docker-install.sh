#!/usr/bin/env bash
set -e
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
echo "=== Android Control — Universal Docker Install (any phone) ==="

# 1. Check Docker
if ! command -v docker &>/dev/null; then
  echo "Docker not found. Install Docker Desktop / docker.io first."
  echo "  Ubuntu: sudo apt update && sudo apt install docker.io docker-compose-plugin"
  exit 1
fi

# 2. Check USB / ADB
echo "[1/4] Checking ADB..."
if command -v adb &>/dev/null; then
  adb devices -l || true
else
  echo "ADB not found on host, will use Docker adb-bridge"
fi

# 3. Build desktop image (universal, works for any phone)
echo "[2/4] Building android-control Docker image (Qt6/C++20 + scrcpy)..."
docker build -t android-control:0.1.0 -f "$ROOT_DIR/Dockerfile" "$ROOT_DIR"

# 4. Build APK server (universal APK for any phone, minSdk 24)
echo "[3/4] Starting APK download server (http://localhost:8080/app-debug.apk)..."
docker compose -f "$ROOT_DIR/docker-compose.yml" up -d apk-server 2>/dev/null || \
  docker-compose -f "$ROOT_DIR/docker-compose.yml" up -d apk-server 2>/dev/null || \
  echo "Start apk-server manually: docker run -p 8080:8080 -v $ROOT_DIR/android/app/build/outputs/apk/debug:/apk python:3.11-alpine sh -c 'cd /apk && python -m http.server 8080'"

# 5. Instructions
cat <<'EOS'

=== INSTALL FOR ANY PHONE (universal) ===

[Phone] Universal APK (Android 7.0+ — any manufacturer):
  Direct (phone browser): https://github.com/s-a-m-y1/android-control/releases/download/v0.1.0/app-debug.apk
  Local Docker:           http://localhost:8080/app-debug.apk  (open on phone same WiFi)
  ADB (USB):              adb install android/app/build/outputs/apk/debug/app-debug.apk
  Docker ADB:             docker run --rm --privileged -v /dev/bus/usb:/dev/bus/usb -v $PWD:/apk instrumentisto/adb adb -s <serial> install /apk/android/app/build/outputs/apk/debug/app-debug.apk

[Phone] Enable USB Debugging (any phone):
  1. Settings → About phone → tap Build number 7× → Developer options enabled
  2. Settings → Developer options → enable USB Debugging
  3. Connect USB → tap Allow on "Allow USB debugging?" → never bypass this

[Desktop] Run Control GUI (any phone via USB):
  Host install:  ./scripts/setup.sh && ./build/android-control
  Docker GUI:    xhost +local:docker
                 docker compose -f docker-compose.yml run --rm --privileged -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v /dev/bus/usb:/dev/bus/usb android-control-desktop
                 # or: docker run --rm -it --privileged -v /dev/bus/usb:/dev/bus/usb -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix android-control:0.1.0

[Desktop] Test connection (any phone):
  adb devices -l
  # should show: <serial> device usb:..., model:..., battery via dumpsys

EOS

# 6. Quick test with connected device
echo "[4/4] Testing connected device (if any)..."
if command -v adb &>/dev/null; then
  adb devices -l
  SERIAL=$(adb devices | awk 'NR>1 && $2=="device" {print $1; exit}')
  if [ -n "$SERIAL" ]; then
    echo "Found device: $SERIAL"
    echo -n "Installing APK to $SERIAL... "
    adb -s "$SERIAL" install -r "$ROOT_DIR/android/app/build/outputs/apk/debug/app-debug.apk" 2>&1 | tail -1
    echo "Launching..."
    adb -s "$SERIAL" shell am start -n com.contentfilter.app/.SplashActivity 2>&1 | head -1
    echo "Done. Open Control tab → Start Service on phone."
  else
    echo "No device connected. Connect any phone via USB and enable USB Debugging."
  fi
fi

echo "=== Done. See README.md and docs/installation/README.md ==="
