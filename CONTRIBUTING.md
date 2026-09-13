# Contributing

## Workflow
1. Fork, branch from `main`, commit with conventional messages
2. Run `./scripts/build.sh` and `./scripts/test.sh` before push
3. For desktop: `cmake -S desktop -B build && cmake --build build && ctest`
4. For Android: `cd android && ./gradlew testDebugUnitTest`

## Commit style
- `feat: add clipboard sync`
- `fix: handle unauthorized device`
- `docs: update installation`
- `test: add adb manager tests`

## Security
- Never bypass ADB authorization, never collect data without consent
- Minimal permissions; explain each permission in PR

## Code style
- C++20, clang-format, Qt6 idioms, spdlog, GoogleTest
- Kotlin, Jetpack, Material, MVVM where applicable
