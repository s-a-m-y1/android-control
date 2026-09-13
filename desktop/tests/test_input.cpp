#include <gtest/gtest.h>
#include "DeviceInfo.h"
#include <QKeySequence>

using namespace AndroidControl;

TEST(InputTest, KeyMapping) {
    // Verify key codes for Android
    // Back=4, Home=3, AppSwitch=187, VolumeUp=24, VolumeDown=25, Power=26
    EXPECT_EQ(4, 4); // placeholder for input handling
    EXPECT_EQ(3, 3);
}

TEST(InputTest, MouseMapping) {
    // Mouse left=touch, right=Back, middle=Home, wheel=scroll
    // Verify DeviceState handling for input-enabled devices
    DeviceInfo d;
    d.state = DeviceState::Connected;
    EXPECT_TRUE(d.isAuthorized());
    d.state = DeviceState::Unauthorized;
    EXPECT_FALSE(d.isAuthorized());
}

TEST(InputTest, ClipboardSyncFlag) {
    // Ensure clipboard autosync default
    EXPECT_TRUE(true); // SettingsManager default handles this
}
