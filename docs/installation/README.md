# Installation

## Requirements
- Ubuntu 24.04+
- Qt6 (6.2+), CMake 3.16+, g++ C++20, pkg-config
- Android Platform Tools (adb), scrcpy, ffmpeg (optional), spdlog, fmt
- Android 7.0+ (API 24+) for companion

## Automated
```bash
./scripts/setup.sh
# detects OS, arch, installs deps (sudo if available), builds desktop & android
```

## Manual Ubuntu
```bash
sudo apt update
sudo apt install cmake qt6-base-dev libspdlog-dev libfmt-dev libgtest-dev android-sdk-platform-tools scrcpy ffmpeg pkg-config build-essential
./scripts/build.sh
./build/android-control
# install launcher
sudo cp packaging/desktop-entry/com.github.androidcontrol.desktop /usr/share/applications/
sudo cp desktop/resources/icons/android-control.svg /usr/share/icons/hicolor/scalable/apps/
sudo update-desktop-database
```

## Android
1. Enable Developer options: Settings → About phone → tap Build number 7×
2. Enable USB Debugging in Developer options
3. Connect via USB, tap Allow on "Allow USB debugging?"
4. Install APK: `adb install android/app/build/outputs/apk/debug/app-debug.apk` or via Android Studio
5. Open "حِصن" → Control tab → Start Service

## Verify
```bash
adb devices -l          # should show "device"
scrcpy --version
./build/android-control # GUI launches, shows device
./build/tests/android_control_tests
```
