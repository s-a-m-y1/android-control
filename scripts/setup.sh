#!/usr/bin/env bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
echo "=== Android Control Setup ==="
echo "Root: $ROOT_DIR"

# Detect Ubuntu
if [ -f /etc/os-release ]; then
    . /etc/os-release
    echo "OS: $NAME $VERSION"
    if [[ "$ID" != "ubuntu" ]]; then
        echo "Warning: Not Ubuntu ($ID), but continuing..."
    fi
else
    echo "Warning: cannot detect OS"
fi

ARCH=$(uname -m)
echo "Arch: $ARCH"

# Detect ADB
if command -v adb &>/dev/null; then
    echo "ADB: $(adb version | head -1)"
else
    echo "ADB not found. Will install dependencies."
fi

# Detect scrcpy
if command -v scrcpy &>/dev/null; then
    echo "scrcpy: $(scrcpy --version | head -1)"
else
    echo "scrcpy not found. Will install."
fi

# Install deps if missing (requires sudo)
NEEDS_INSTALL=false
for pkg in cmake qt6-base-dev libspdlog-dev libgtest-dev android-sdk-platform-tools scrcpy ffmpeg; do
    if ! dpkg -l | grep -q "^ii  $pkg "; then
        # check via apt list
        if ! dpkg -l | grep -q "$pkg"; then
            echo "Missing: $pkg"
            NEEDS_INSTALL=true
        fi
    fi
done

if [ "$NEEDS_INSTALL" = true ]; then
    echo "Installing missing dependencies (requires sudo)..."
    if [ "$EUID" -eq 0 ]; then
        apt update && apt install -y cmake qt6-base-dev libspdlog-dev libgtest-dev android-sdk-platform-tools scrcpy ffmpeg pkg-config build-essential
    elif command -v sudo &>/dev/null; then
        sudo apt update && sudo apt install -y cmake qt6-base-dev libspdlog-dev libgtest-dev android-sdk-platform-tools scrcpy ffmpeg pkg-config build-essential
    else
        echo "No sudo. Please manually install: cmake qt6-base-dev libspdlog-dev libgtest-dev android-sdk-platform-tools scrcpy ffmpeg"
    fi
else
    echo "All dependencies present."
fi

# Build desktop
echo "Building desktop..."
"$SCRIPT_DIR/build.sh" || echo "Desktop build failed, check logs"

# Build Android APK if gradle present
if [ -f "$ROOT_DIR/android/gradlew" ]; then
    echo "Building Android APK..."
    (cd "$ROOT_DIR/android" && ./gradlew assembleDebug --info) || echo "Android build failed"
    APK=$(find "$ROOT_DIR/android" -name "*.apk" | head -1)
    if [ -n "$APK" ]; then
        echo "APK built: $APK"
    fi
else
    echo "No gradlew found, skipping Android build"
fi

# Prepare packaging
echo "Preparing packaging..."
mkdir -p "$ROOT_DIR/build"
if [ -f "$ROOT_DIR/build/android-control" ]; then
    echo "Desktop binary: $ROOT_DIR/build/android-control"
fi

echo "Setup complete."
echo "Launch desktop: $ROOT_DIR/build/android-control  or  ./scripts/build.sh && ./build/android-control"
echo "Install desktop entry: cp packaging/desktop-entry/com.github.androidcontrol.desktop ~/.local/share/applications/"
