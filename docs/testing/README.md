# Testing

## Desktop (GoogleTest, C++20, Qt6)

### Unit
- `tests/test_device_info.cpp` — DeviceInfo displayName, state mapping
- `tests/test_adb_manager.cpp` — ADB detection, listDevices, multiple devices
- `tests/test_settings.cpp` — defaults, save/load, effectiveBitrate
- `tests/test_scrcpy.cpp` — isInstalled, buildArgs, mirroring state
- `tests/test_input.cpp` — input mapping, clipboard flag

Run:
```bash
./scripts/test.sh
# or
cmake -S desktop -B build && cmake --build build && ctest --test-dir build --output-on-failure
./build/tests/android_control_tests
```

Result (real run on Ubuntu 26.04, Realme RMX3760 connected):
```
[==========] 18 tests from 5 suites
[  PASSED  ] 18 tests.
```

### Real Device (hardware required)
If adb device available:
- Detect, authorize, install APK, launch desktop, mirror, mouse/keyboard, Back/Home, clipboard, screenshot, recording, file push/pull, disconnect/reconnect, error scenarios

If no hardware: marked `NOT TESTED — Android device required`

## Android (JUnit, AndroidX Test)

- `src/test/java/.../DeviceInfoProviderTest.kt` — displayName, fallback
- `src/test/java/.../ControlServiceTest.kt` — constants
- `src/androidTest/java/.../ControlInstrumentedTest.kt` — provider, lifecycle (needs emulator/device)

Run:
```bash
cd android && ./gradlew testDebugUnitTest
./gradlew connectedDebugAndroidTest  # needs device/emulator
```

## CI
- `.github/workflows/desktop-build.yml` — builds desktop, runs tests
- `.github/workflows/android-build.yml` — builds Android, runs unit tests
- `.github/workflows/tests.yml` — combined
