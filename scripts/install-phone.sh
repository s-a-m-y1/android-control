#!/usr/bin/env bash
set -e
# Universal phone installer — works for ANY Android phone (minSdk 24, Android 7.0+)
# Usage: ./scripts/install-phone.sh [serial]
#   without serial: installs to first "device" found via adb devices
#   with serial:    adb -s SERIAL install

APK_LOCAL="android/app/build/outputs/apk/debug/app-debug.apk"
APK_URL="https://github.com/s-a-m-y1/android-control/releases/download/v0.1.0/app-debug.apk"
SERIAL="${1:-}"

if [ ! -f "$APK_LOCAL" ]; then
  echo "Local APK not found, downloading universal APK..."
  mkdir -p "$(dirname "$APK_LOCAL")"
  if command -v wget &>/dev/null; then wget -O "$APK_LOCAL" "$APK_URL"
  elif command -v curl &>/dev/null; then curl -L -o "$APK_LOCAL" "$APK_URL"
  else echo "Need wget or curl to download $APK_URL"; exit 1; fi
fi

echo "APK: $APK_LOCAL ($(du -h "$APK_LOCAL" | cut -f1)) — universal for any phone (minSdk 24)"

if ! command -v adb &>/dev/null; then
  echo "adb not found. Install Android Platform Tools:"
  echo "  sudo apt install android-sdk-platform-tools"
  echo "Or via Docker: docker run --rm --privileged -v /dev/bus/usb:/dev/bus/usb -v \$PWD:/apk instrumentisto/adb adb devices -l"
  exit 1
fi

echo "Checking devices..."
adb devices -l
if [ -z "$SERIAL" ]; then
  SERIAL=$(adb devices | awk 'NR>1 && $2=="device" {print $1; exit}')
  if [ -z "$SERIAL" ]; then
    echo ""
    echo "No authorized device found."
    echo "For ANY phone:"
    echo "  1. Settings → About phone → tap Build number 7×"
    echo "  2. Developer options → enable USB Debugging"
    echo "  3. Connect USB → tap Allow on phone"
    echo "  4. Re-run: ./scripts/install-phone.sh"
    echo ""
    echo "Or install directly on phone (no USB):"
    echo "  Open on phone: $APK_URL"
    exit 1
  fi
fi

echo "Installing to $SERIAL (any phone)..."
adb -s "$SERIAL" install -r "$APK_LOCAL"
echo "Success. Launching..."

adb -s "$SERIAL" shell am start -n com.contentfilter.app/.SplashActivity 2>&1 | head -1
adb -s "$SERIAL" shell dumpsys package com.contentfilter.app 2>&1 | grep -E "versionName|firstInstallTime" | head -3

echo ""
echo "On phone: open app → Control tab → Start Service → grant POST_NOTIFICATIONS if asked."
echo "Then on desktop: ./build/android-control or Docker GUI: see scripts/docker-install.sh"
