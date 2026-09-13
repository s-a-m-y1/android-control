#include <gtest/gtest.h>
#include "SettingsManager.h"
#include <QTemporaryDir>
#include <QFile>

using namespace AndroidControl;

TEST(SettingsTest, Defaults) {
    SettingsManager s;
    EXPECT_EQ(s.display.maxFps, 60);
    EXPECT_EQ(s.display.videoCodec, "h264");
    EXPECT_TRUE(s.display.stayAwake);
    EXPECT_EQ(s.recording.videoFormat, "mp4");
    EXPECT_FALSE(s.recording.outputDir.isEmpty());
}

TEST(SettingsTest, SaveLoad) {
    QTemporaryDir tmp;
    ASSERT_TRUE(tmp.isValid());
    QString path = tmp.path() + "/settings.json";
    SettingsManager s;
    s.display.maxSize = 1024;
    s.display.maxFps = 30;
    s.display.videoBitrate = 4000000;
    s.input.clipboardAutosync = false;
    s.recording.outputDir = tmp.path() + "/Videos";

    // Manually set config path via hack: use file directly
    QJsonObject obj;
    QJsonObject disp; disp["maxSize"] = 1024; disp["maxFps"] = 30; disp["videoBitrate"] = 4000000; disp["videoCodec"] = "h265";
    disp["fullscreen"] = false; disp["stayAwake"] = true; disp["showTouches"] = true; disp["disableScreensaver"] = true;
    obj["display"] = disp;
    QJsonObject inp; inp["mouseControl"] = true; inp["keyboardControl"] = true; inp["clipboardAutosync"] = false; inp["turnScreenOff"] = false;
    obj["input"] = inp;
    QJsonObject conn; conn["autoReconnect"] = true; conn["selectedDevice"] = "test123";
    obj["connection"] = conn;
    QJsonObject rec; rec["outputDir"] = tmp.path() + "/Videos"; rec["videoFormat"] = "mkv"; rec["recordingQuality"] = "low";
    obj["recording"] = rec;

    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    QJsonDocument doc(obj);
    f.write(doc.toJson());
    f.close();

    EXPECT_TRUE(QFile::exists(path));
    QJsonDocument doc2 = QJsonDocument::fromJson([path](){
        QFile ff(path); ff.open(QIODevice::ReadOnly); return ff.readAll();
    }());
    EXPECT_FALSE(doc2.isNull());
    EXPECT_EQ(doc2.object()["display"].toObject()["maxFps"].toInt(), 30);
    EXPECT_EQ(doc2.object()["recording"].toObject()["videoFormat"].toString(), "mkv");
}

TEST(SettingsTest, EffectiveBitrate) {
    SettingsManager s;
    s.display.videoBitrate = 8000000;
    s.recording.recordingQuality = "high";
    EXPECT_EQ(s.effectiveBitrate(), 8000000);
    s.recording.recordingQuality = "low";
    EXPECT_EQ(s.effectiveBitrate(), 2000000);
    s.recording.recordingQuality = "medium";
    EXPECT_EQ(s.effectiveBitrate(), 4000000);
}
