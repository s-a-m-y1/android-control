#include <gtest/gtest.h>
#include "DeviceInfo.h"

using namespace AndroidControl;

TEST(DeviceInfoTest, StatusMapping) {
    EXPECT_EQ(deviceStateFromString("device"), DeviceState::Connected);
    EXPECT_EQ(deviceStateFromString("unauthorized"), DeviceState::Unauthorized);
    EXPECT_EQ(deviceStateFromString("offline"), DeviceState::Offline);
    EXPECT_EQ(deviceStateToString(DeviceState::Connected), "Connected");
    EXPECT_EQ(deviceStateToString(DeviceState::Unauthorized), "Unauthorized");
}

TEST(DeviceInfoTest, DisplayName) {
    DeviceInfo d;
    d.serial = "ABC123";
    d.model = "Pixel 7";
    d.manufacturer = "google";
    EXPECT_EQ(d.displayName(), "Google Pixel 7");
    d.model = "";
    EXPECT_EQ(d.displayName(), "ABC123");
}

TEST(DeviceInfoTest, IsConnected) {
    DeviceInfo d;
    d.state = DeviceState::Connected;
    EXPECT_TRUE(d.isConnected());
    d.state = DeviceState::Unauthorized;
    EXPECT_FALSE(d.isConnected());
}

TEST(DeviceInfoTest, ResolutionParsing) {
    DeviceInfo d;
    d.resolution = "1080x2400";
    EXPECT_EQ(d.resolution, "1080x2400");
    d.batteryLevel = 82;
    EXPECT_EQ(d.batteryLevel, 82);
}
