#pragma once
#include <QObject>
#include <QString>
#include <QProcess>

namespace AndroidControl {

class FileTransferManager : public QObject {
    Q_OBJECT
public:
    explicit FileTransferManager(QObject *parent = nullptr);

    // PC -> Android: adb push
    bool pushFile(const QString &serial, const QString &localPath, const QString &remotePath);
    // Android -> PC: adb pull
    bool pullFile(const QString &serial, const QString &remotePath, const QString &localPath);
    // List files via adb shell ls
    QStringList listRemoteFiles(const QString &serial, const QString &remoteDir);

    bool cancel();
    bool isBusy() const { return m_busy; }

signals:
    void progress(int percent);
    void transferFinished(const QString &message);
    void transferFailed(const QString &error);
    void transferProgress(const QString &serial, int percent);

private:
    QProcess *m_process = nullptr;
    bool m_busy = false;
    bool m_cancelRequested = false;
};

} // namespace AndroidControl
