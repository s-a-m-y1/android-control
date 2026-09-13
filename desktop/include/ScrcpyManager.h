#pragma once
#include "SettingsManager.h"
#include <QObject>
#include <QProcess>
#include <QMap>
#include <QString>

namespace AndroidControl {

class ScrcpyManager : public QObject {
    Q_OBJECT
public:
    explicit ScrcpyManager(SettingsManager *settings, QObject *parent = nullptr);
    ~ScrcpyManager() override;

    static bool isScrcpyInstalled();
    static QString scrcpyVersion();

    bool isMirroring(const QString &serial) const;
    bool start(const QString &serial, const QStringList &extraArgs = {});
    bool stop(const QString &serial);
    void stopAll();

    // Actions
    bool takeScreenshot(const QString &serial, const QString &outputPath = {});
    bool startRecording(const QString &serial, const QString &outputPath = {});
    bool sendKeyEvent(const QString &serial, int keyCode);
    bool rotateDevice(const QString &serial);

    void updateSettings(SettingsManager *settings) { m_settings = settings; }

    QString lastError() const { return m_lastError; }

signals:
    void mirroringStarted(const QString &serial);
    void mirroringStopped(const QString &serial);
    void mirroringFailed(const QString &serial, const QString &error);
    void screenshotTaken(const QString &path);
    void recordingStarted(const QString &path);
    void recordingStopped();

private:
    QStringList buildArgs(const QString &serial, const QStringList &extraArgs = {}) const;
    QString defaultScreenshotPath(const QString &serial) const;
    QString defaultRecordPath(const QString &serial) const;

    SettingsManager *m_settings = nullptr;
    QMap<QString, QProcess*> m_processes;
    // For recording vs mirroring distinction
    QMap<QString, bool> m_isRecording;
    QString m_lastError;
    QProcess* getProcess(const QString &serial) const;
};

} // namespace AndroidControl
