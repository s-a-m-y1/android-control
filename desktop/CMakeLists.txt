cmake_minimum_required(VERSION 3.16)
project(AndroidControl VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Enable warnings
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

# Optional deps
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    message(STATUS "spdlog not found via find_package, trying pkg-config / fallback")
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(SPDLOG spdlog)
    endif()
endif()

# FFmpeg for future hardware accel (optional)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(FFMPEG libavcodec libavformat libavutil libswscale)
    if(FFMPEG_FOUND)
        message(STATUS "FFmpeg found: ${FFMPEG_LIBRARIES}")
        add_definitions(-DHAVE_FFMPEG)
    endif()
endif()

# Core library (for reuse in tests)
set(CORE_SOURCES
    src/AdbManager.cpp
    src/SettingsManager.cpp
    src/ScrcpyManager.cpp
    src/FileTransferManager.cpp
    src/ClipboardManager.cpp
    src/MainWindow.cpp
    src/DeviceWidget.cpp
)

set(CORE_HEADERS
    include/DeviceInfo.h
    include/AdbManager.h
    include/SettingsManager.h
    include/ScrcpyManager.h
    include/FileTransferManager.h
    include/ClipboardManager.h
    include/MainWindow.h
    include/DeviceWidget.h
)

add_library(android_control_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(android_control_core PUBLIC include)
target_link_libraries(android_control_core PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
)

if(TARGET spdlog::spdlog)
    target_link_libraries(android_control_core PUBLIC spdlog::spdlog)
elseif(SPDLOG_FOUND)
    target_include_directories(android_control_core PUBLIC ${SPDLOG_INCLUDE_DIRS})
    target_link_libraries(android_control_core PUBLIC ${SPDLOG_LIBRARIES})
else()
    find_package(fmt QUIET)
    if(TARGET fmt::fmt)
        target_link_libraries(android_control_core PUBLIC fmt::fmt)
    endif()
endif()

if(FFMPEG_FOUND)
    target_include_directories(android_control_core PUBLIC ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(android_control_core PUBLIC ${FFMPEG_LIBRARIES})
endif()

# Main executable
add_executable(android-control src/main.cpp)
target_link_libraries(android-control PRIVATE android_control_core)

if(TARGET spdlog::spdlog)
    target_link_libraries(android-control PRIVATE spdlog::spdlog)
elseif(SPDLOG_FOUND)
    target_include_directories(android-control PRIVATE ${SPDLOG_INCLUDE_DIRS})
    target_link_libraries(android-control PRIVATE ${SPDLOG_LIBRARIES})
else()
    # Fallback: try header-only spdlog via include path /usr/include/spdlog
    message(STATUS "Using system spdlog headers")
    target_link_libraries(android-control PRIVATE fmt)
endif()

if(FFMPEG_FOUND)
    target_include_directories(android-control PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(android-control PRIVATE ${FFMPEG_LIBRARIES})
endif()

# Screenshot generator (for docs)
add_executable(screenshot_gen src/screenshot_gen.cpp)
target_link_libraries(screenshot_gen PRIVATE android_control_core)

# Installation
install(TARGETS android-control DESTINATION bin)
install(FILES packaging/desktop-entry/com.github.androidcontrol.desktop DESTINATION share/applications)
install(FILES resources/icons/android-control.svg DESTINATION share/icons/hicolor/scalable/apps)

# Tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Summary
message(STATUS "Android Control ${PROJECT_VERSION} configured")
message(STATUS "  Qt6: ${Qt6_VERSION}")
message(STATUS "  C++: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
