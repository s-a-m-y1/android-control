#pragma once
#include <QObject>
#include <QString>
#include <QJsonObject>

namespace AndroidControl {

struct DisplaySettings {
    int maxSize = 0; // 0 = original
    int maxFps = 60;
    int videoBitrate = 8000000; // 8 Mbps
    QString videoCodec = "h264"; // h264, h265, av1
    bool fullscreen = false;
    bool stayAwake = true;
    bool showTouches = false;
    bool disableScreensaver = true;
};

struct InputSettings {
    bool mouseControl = true;
    bool keyboardControl = true;
    bool clipboardAutosync = true;
    bool turnScreenOff = false;
};

struct ConnectionSettings {
    bool autoReconnect = true;
    QString selectedDevice;
};

struct RecordingSettings {
    QString outputDir;
    QString videoFormat = "mp4"; // mp4, mkv
    QString recordingQuality = "high"; // high, medium, low
};

class SettingsManager : public QObject {
    Q_OBJECT
public:
    explicit SettingsManager(QObject *parent = nullptr);
    ~SettingsManager() override = default;

    DisplaySettings display;
    InputSettings input;
    ConnectionSettings connection;
    RecordingSettings recording;

    void load();
    void save();
    QString configPath() const;

    // Helpers for scrcpy args
    int effectiveBitrate() const;

signals:
    void settingsChanged();

private:
    QString m_configPath;
    void setDefaults();
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &obj);
};

} // namespace AndroidControl
