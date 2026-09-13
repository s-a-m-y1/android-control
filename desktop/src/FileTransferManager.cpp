#include "FileTransferManager.h"
#include <QProcess>
#include <QFileInfo>
#include <spdlog/spdlog.h>

namespace AndroidControl {

FileTransferManager::FileTransferManager(QObject *parent) : QObject(parent) {
    m_process = new QProcess(this);
}

bool FileTransferManager::pushFile(const QString &serial, const QString &localPath, const QString &remotePath) {
    if (m_busy) {
        emit transferFailed("Another transfer in progress");
        return false;
    }
    QFileInfo fi(localPath);
    if (!fi.exists()) {
        emit transferFailed("Local file does not exist: " + localPath);
        return false;
    }
    m_busy = true;
    m_cancelRequested = false;
    spdlog::info("Push {} to {}:{}", localPath.toStdString(), serial.toStdString(), remotePath.toStdString());

    QProcess p;
    p.start("adb", {"-s", serial, "push", localPath, remotePath});
    // Simple progress: wait with polling? For now wait
    if (!p.waitForFinished(120000)) {
        m_busy = false;
        emit transferFailed("Push timed out");
        return false;
    }
    m_busy = false;
    if (p.exitCode() != 0) {
        QString err = QString::fromUtf8(p.readAllStandardError());
        emit transferFailed(err.isEmpty() ? "Push failed" : err);
        return false;
    }
    emit transferFinished("Push completed: " + remotePath);
    return true;
}

bool FileTransferManager::pullFile(const QString &serial, const QString &remotePath, const QString &localPath) {
    if (m_busy) {
        emit transferFailed("Another transfer in progress");
        return false;
    }
    m_busy = true;
    spdlog::info("Pull {}:{} to {}", serial.toStdString(), remotePath.toStdString(), localPath.toStdString());
    QProcess p;
    p.start("adb", {"-s", serial, "pull", remotePath, localPath});
    if (!p.waitForFinished(120000)) {
        m_busy = false;
        emit transferFailed("Pull timed out");
        return false;
    }
    m_busy = false;
    if (p.exitCode() != 0) {
        QString err = QString::fromUtf8(p.readAllStandardError());
        emit transferFailed(err.isEmpty() ? "Pull failed" : err);
        return false;
    }
    emit transferFinished("Pull completed: " + localPath);
    return true;
}

QStringList FileTransferManager::listRemoteFiles(const QString &serial, const QString &remoteDir) {
    QProcess p;
    p.start("adb", {"-s", serial, "shell", "ls", "-1", remoteDir});
    if (!p.waitForFinished(5000)) return {};
    if (p.exitCode() != 0) return {};
    QString out = QString::fromUtf8(p.readAllStandardOutput());
    return out.split('\n', Qt::SkipEmptyParts);
}

bool FileTransferManager::cancel() {
    if (m_process && m_process->state() == QProcess::Running) {
        m_cancelRequested = true;
        m_process->kill();
        m_busy = false;
        emit transferFailed("Cancelled");
        return true;
    }
    return false;
}

} // namespace AndroidControl
