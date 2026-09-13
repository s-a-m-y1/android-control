find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    set(GTEST_LIBRARIES gtest gtest_main)
    set(GTEST_INCLUDE_DIRS "")
endif()

set(TEST_SOURCES
    test_adb_manager.cpp
    test_device_info.cpp
    test_settings.cpp
    test_scrcpy.cpp
    test_input.cpp
)

add_executable(android_control_tests ${TEST_SOURCES})

target_include_directories(android_control_tests PRIVATE
    ../include
    ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(android_control_tests PRIVATE
    android_control_core
)

if(TARGET spdlog::spdlog)
    target_link_libraries(android_control_tests PRIVATE spdlog::spdlog)
elseif(SPDLOG_FOUND)
    target_include_directories(android_control_tests PRIVATE ${SPDLOG_INCLUDE_DIRS})
    target_link_libraries(android_control_tests PRIVATE ${SPDLOG_LIBRARIES})
else()
    find_package(fmt QUIET)
    if(TARGET fmt::fmt)
        target_link_libraries(android_control_tests PRIVATE fmt::fmt)
    endif()
endif()

if(TARGET GTest::gtest)
    target_link_libraries(android_control_tests PRIVATE GTest::gtest GTest::gtest_main)
else()
    target_link_libraries(android_control_tests PRIVATE gtest gtest_main pthread)
endif()

include(GoogleTest)
gtest_discover_tests(android_control_tests)
