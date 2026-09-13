#!/usr/bin/env bash
set -e
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
CMAKE_ARGS="${CMAKE_ARGS:-}"

echo "=== Building Android Control Desktop (C++ Qt6) ==="
mkdir -p "$BUILD_DIR"
# Use local cmake if system cmake not found
CMAKE_BIN="cmake"
if ! command -v cmake &>/dev/null; then
    if [ -x "/tmp/cmake_local2/usr/bin/cmake" ]; then
        CMAKE_BIN="/tmp/cmake_local2/usr/bin/cmake"
        export LD_LIBRARY_PATH="/tmp/cmake_local2/usr/lib/x86_64-linux-gnu:/tmp/cmake_local2/usr/lib:${LD_LIBRARY_PATH:-}"
    elif [ -x "/tmp/cmake.tar.gz" ]; then
        echo "cmake not found, please install cmake"
        exit 1
    fi
fi

# Handle sysroot for rootless builds
EXTRA_ARGS=""
if [ -d "/tmp/sysroot/usr" ]; then
    EXTRA_ARGS="-DCMAKE_PREFIX_PATH=/tmp/sysroot/usr;/tmp/sysroot/usr/lib/x86_64-linux-gnu/cmake;/usr"
fi

$CMAKE_BIN -S "$ROOT_DIR/desktop" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release $EXTRA_ARGS $CMAKE_ARGS
$CMAKE_BIN --build "$BUILD_DIR" --parallel "$(nproc)"
echo "Build complete: $BUILD_DIR/android-control"
ls -lh "$BUILD_DIR/android-control" 2>/dev/null || true

echo "Building Android APK..."
if [ -f "$ROOT_DIR/android/gradlew" ]; then
    (cd "$ROOT_DIR/android" && ./gradlew assembleDebug)
    find "$ROOT_DIR/android" -name "*.apk" -type f | head -5
else
    echo "No android/gradlew, skipping"
fi
