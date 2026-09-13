# Architecture

```
GUI (Qt6 Widgets)
 │
 ├── Device Manager → ADB (adb devices, getprop, dumpsys)
 │    └── AdbManager (src/AdbManager.cpp, include/AdbManager.h)
 │
 ├── Mirror Engine → scrcpy (QProcess, hardware-accel)
 │    └── ScrcpyManager (src/ScrcpyManager.cpp)
 │
 ├── Input Controller → Mouse + Keyboard → adb input keyevent + scrcpy
 │
 ├── FileTransferManager → adb push/pull
 ├── ClipboardManager → QClipboard + dumpsys clipboard + scrcpy --clipboard-autosync
 └── SettingsManager → JSON at ~/.config/android-control/settings.json
```

## Desktop
- C++20, Qt6 (Core, Widgets, Gui, Network), CMake, spdlog, GoogleTest, FFmpeg optional
- `MainWindow` with DeviceWidget list, preview frame, status bar, action buttons
- Settings dialog (Display/Input/Connection/Recording)

## Android Companion
- Kotlin, Jetpack, Material, Room, Retrofit, WorkManager
- `ControlService` (foreground, specialUse), `DeviceInfoProvider`, `ControlFragment`
- No bypass of security; only ADB-authorized USB connection

## Data Flow
1. Desktop polls `adb devices -l` every 2s
2. On Mirror, spawns `scrcpy --serial <id> --max-size --max-fps --video-bit-rate --stay-awake`
3. Input: scrcpy handles mouse/keyboard → touch; desktop also sends `adb shell input keyevent`
4. Screenshot: `adb exec-out screencap -p`
5. Recording: `scrcpy --record <file>`
6. File transfer/clipboard via adb + scrcpy autosync
