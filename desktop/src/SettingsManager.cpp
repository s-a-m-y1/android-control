#include "SettingsManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <spdlog/spdlog.h>

namespace AndroidControl {

SettingsManager::SettingsManager(QObject *parent) : QObject(parent) {
    QString cfg = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    if (cfg.isEmpty()) cfg = QDir::homePath() + "/.config";
    m_configPath = cfg + "/android-control/settings.json";
    setDefaults();
    load();
}

void SettingsManager::setDefaults() {
    display = DisplaySettings();
    input = InputSettings();
    connection = ConnectionSettings();
    recording = RecordingSettings();
    QString home = QDir::homePath();
    QString vids = home + "/Videos/AndroidControl";
    // Use Videos if exists
    if (QDir(home + "/Videos").exists()) {
        recording.outputDir = vids;
    } else {
        recording.outputDir = home + "/Videos";
    }
}

QString SettingsManager::configPath() const { return m_configPath; }

int SettingsManager::effectiveBitrate() const {
    if (recording.recordingQuality == "low") return 2000000;
    if (recording.recordingQuality == "medium") return 4000000;
    return display.videoBitrate;
}

void SettingsManager::load() {
    QFile f(m_configPath);
    if (!f.exists()) {
        setDefaults();
        return;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        spdlog::warn("Failed to open settings file {}", m_configPath.toStdString());
        return;
    }
    QByteArray data = f.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        spdlog::warn("Invalid settings json");
        return;
    }
    fromJson(doc.object());
}

void SettingsManager::save() {
    QDir().mkpath(QFileInfo(m_configPath).absolutePath());
    QFile f(m_configPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        spdlog::error("Failed to save settings to {}", m_configPath.toStdString());
        return;
    }
    QJsonDocument doc(toJson());
    f.write(doc.toJson(QJsonDocument::Indented));
    emit settingsChanged();
}

QJsonObject SettingsManager::toJson() const {
    QJsonObject obj;
    QJsonObject disp;
    disp["maxSize"] = display.maxSize;
    disp["maxFps"] = display.maxFps;
    disp["videoBitrate"] = display.videoBitrate;
    disp["videoCodec"] = display.videoCodec;
    disp["fullscreen"] = display.fullscreen;
    disp["stayAwake"] = display.stayAwake;
    disp["showTouches"] = display.showTouches;
    disp["disableScreensaver"] = display.disableScreensaver;
    obj["display"] = disp;

    QJsonObject inp;
    inp["mouseControl"] = input.mouseControl;
    inp["keyboardControl"] = input.keyboardControl;
    inp["clipboardAutosync"] = input.clipboardAutosync;
    inp["turnScreenOff"] = input.turnScreenOff;
    obj["input"] = inp;

    QJsonObject conn;
    conn["autoReconnect"] = connection.autoReconnect;
    conn["selectedDevice"] = connection.selectedDevice;
    obj["connection"] = conn;

    QJsonObject rec;
    rec["outputDir"] = recording.outputDir;
    rec["videoFormat"] = recording.videoFormat;
    rec["recordingQuality"] = recording.recordingQuality;
    obj["recording"] = rec;

    return obj;
}

void SettingsManager::fromJson(const QJsonObject &obj) {
    if (obj.contains("display") && obj["display"].isObject()) {
        auto d = obj["display"].toObject();
        display.maxSize = d.value("maxSize").toInt(display.maxSize);
        display.maxFps = d.value("maxFps").toInt(display.maxFps);
        display.videoBitrate = d.value("videoBitrate").toInt(display.videoBitrate);
        display.videoCodec = d.value("videoCodec").toString(display.videoCodec);
        display.fullscreen = d.value("fullscreen").toBool(display.fullscreen);
        display.stayAwake = d.value("stayAwake").toBool(display.stayAwake);
        display.showTouches = d.value("showTouches").toBool(display.showTouches);
        display.disableScreensaver = d.value("disableScreensaver").toBool(display.disableScreensaver);
    }
    if (obj.contains("input") && obj["input"].isObject()) {
        auto d = obj["input"].toObject();
        input.mouseControl = d.value("mouseControl").toBool(input.mouseControl);
        input.keyboardControl = d.value("keyboardControl").toBool(input.keyboardControl);
        input.clipboardAutosync = d.value("clipboardAutosync").toBool(input.clipboardAutosync);
        input.turnScreenOff = d.value("turnScreenOff").toBool(input.turnScreenOff);
    }
    if (obj.contains("connection") && obj["connection"].isObject()) {
        auto d = obj["connection"].toObject();
        connection.autoReconnect = d.value("autoReconnect").toBool(connection.autoReconnect);
        connection.selectedDevice = d.value("selectedDevice").toString(connection.selectedDevice);
    }
    if (obj.contains("recording") && obj["recording"].isObject()) {
        auto d = obj["recording"].toObject();
        recording.outputDir = d.value("outputDir").toString(recording.outputDir);
        recording.videoFormat = d.value("videoFormat").toString(recording.videoFormat);
        recording.recordingQuality = d.value("recordingQuality").toString(recording.recordingQuality);
    }
    if (recording.outputDir.isEmpty()) {
        recording.outputDir = QDir::homePath() + "/Videos/AndroidControl";
    }
}

} // namespace AndroidControl
