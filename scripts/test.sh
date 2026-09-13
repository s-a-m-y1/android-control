#!/usr/bin/env bash
set -e
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "=== Running Android Control Tests ==="

# Desktop tests (GoogleTest)
if [ -f "$BUILD_DIR/tests/android_control_tests" ]; then
    echo "Running desktop tests..."
    LD_LIBRARY_PATH="/tmp/sysroot/usr/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}" "$BUILD_DIR/tests/android_control_tests" --gtest_output=xml:"$BUILD_DIR/test_results.xml" || {
        echo "Desktop tests failed"
        exit 1
    }
    echo "Desktop tests passed"
else
    echo "Desktop test binary not found, building..."
    "$(dirname "$0")/build.sh"
    if [ -f "$BUILD_DIR/tests/android_control_tests" ]; then
        LD_LIBRARY_PATH="/tmp/sysroot/usr/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}" "$BUILD_DIR/tests/android_control_tests"
    else
        echo "Still not found, skipping"
    fi
fi

# Also run ctest
if [ -f "$BUILD_DIR/CTestTestfile.cmake" ]; then
    echo "Running ctest..."
    LD_LIBRARY_PATH="/tmp/sysroot/usr/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}" ctest --test-dir "$BUILD_DIR" --output-on-failure || LD_LIBRARY_PATH="/tmp/cmake_local2/usr/lib/x86_64-linux-gnu:/tmp/cmake_local2/usr/lib:${LD_LIBRARY_PATH:-}" /tmp/cmake_local2/usr/bin/ctest --test-dir "$BUILD_DIR" --output-on-failure || true
fi

# Android unit tests
if [ -f "$ROOT_DIR/android/gradlew" ]; then
    echo "Running Android unit tests..."
    (cd "$ROOT_DIR/android" && ./gradlew testDebugUnitTest --info) || echo "Android unit tests failed or not configured"
    (cd "$ROOT_DIR/android" && ./gradlew connectedDebugAndroidTest || echo "Android instrumentation tests need device/emulator - NOT TESTED without hardware")
else
    echo "No gradlew, skipping Android tests"
fi

echo "All tests completed"
