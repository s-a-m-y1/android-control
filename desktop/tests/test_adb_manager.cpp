#include <gtest/gtest.h>
#include "AdbManager.h"

using namespace AndroidControl;

TEST(AdbManagerTest, AdbInstalledCheck) {
    // Should be true on CI with platform-tools
    bool installed = AdbManager::isAdbInstalled();
    // On test machine we expect true, but allow false in isolated CI without adb
    if (installed) {
        QString ver = AdbManager::adbVersion();
        EXPECT_FALSE(ver.isEmpty());
        EXPECT_TRUE(ver.contains("1.") || ver.contains("version") || ver.size() > 0);
    } else {
        GTEST_SKIP() << "ADB not installed in this environment";
    }
}

TEST(AdbManagerTest, ListDevicesDoesNotThrow) {
    if (!AdbManager::isAdbInstalled()) GTEST_SKIP() << "ADB not installed";
    AdbManager mgr;
    // Should not throw even if no devices
    try {
        auto devs = mgr.listDevices(false);
        // Valid: vector may be empty or contain devices
        EXPECT_GE(devs.size(), 0u);
        for (auto &d : devs) {
            EXPECT_FALSE(d.serial.isEmpty());
        }
    } catch (const std::exception &e) {
        FAIL() << "listDevices threw: " << e.what();
    }
}

TEST(AdbManagerTest, DeviceStates) {
    // Test state parsing via deviceStateFromString
    EXPECT_EQ(deviceStateFromString("device"), DeviceState::Connected);
    EXPECT_EQ(deviceStateFromString("unauthorized"), DeviceState::Unauthorized);
    EXPECT_EQ(deviceStateFromString("offline"), DeviceState::Offline);
    EXPECT_EQ(deviceStateFromString("no permissions"), DeviceState::NoPermissions);
}

TEST(AdbManagerTest, MultipleDevicesHandling) {
    if (!AdbManager::isAdbInstalled()) GTEST_SKIP() << "ADB not installed";
    AdbManager mgr;
    auto devs = mgr.listDevices(false);
    // If we have at least 1 device, test detailed fetch
    for (auto &d : devs) {
        if (d.state == DeviceState::Connected) {
            auto info = mgr.getDeviceInfo(d.serial);
            // Detailed info should have been fetched; if emulation, may be empty but serial must match
            EXPECT_EQ(info.serial, d.serial);
        }
    }
}
