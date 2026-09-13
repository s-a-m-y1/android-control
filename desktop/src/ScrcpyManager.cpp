#include "ScrcpyManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QProcess>
#include <QThread>
#include <QDebug>
#include <spdlog/spdlog.h>

namespace AndroidControl {

ScrcpyManager::ScrcpyManager(SettingsManager *settings, QObject *parent)
    : QObject(parent), m_settings(settings) {}

ScrcpyManager::~ScrcpyManager() { stopAll(); }

bool ScrcpyManager::isScrcpyInstalled() {
    return !QStandardPaths::findExecutable("scrcpy").isEmpty();
}

QString ScrcpyManager::scrcpyVersion() {
    QProcess p;
    p.start("scrcpy", {"--version"});
    if (!p.waitForFinished(3000)) return {};
    return QString::fromUtf8(p.readAllStandardOutput() + p.readAllStandardError()).split('\n').first().trimmed();
}

QStringList ScrcpyManager::buildArgs(const QString &serial, const QStringList &extraArgs) const {
    QStringList args;
    args << "--serial" << serial;

    if (m_settings) {
        auto &d = m_settings->display;
        auto &i = m_settings->input;
        if (d.maxSize > 0) args << "--max-size" << QString::number(d.maxSize);
        if (d.maxFps != 60 && d.maxFps > 0) args << "--max-fps" << QString::number(d.maxFps);
        if (d.videoBitrate > 0) args << "--video-bit-rate" << QString::number(d.videoBitrate);
        if (d.videoCodec != "h264" && !d.videoCodec.isEmpty()) args << "--video-codec" << d.videoCodec;
        if (d.stayAwake) args << "--stay-awake";
        if (d.showTouches) args << "--show-touches";
        if (d.fullscreen) args << "--fullscreen";
        if (d.disableScreensaver) args << "--disable-screensaver";
        if (i.turnScreenOff) args << "--turn-screen-off";
        if (i.clipboardAutosync) args << "--clipboard-autosync";
        else args << "--no-clipboard-autosync";
        args << "--window-title" << QString("Android Control - %1").arg(serial);
    }
    args.append(extraArgs);
    return args;
}

bool ScrcpyManager::isMirroring(const QString &serial) const {
    auto it = m_processes.find(serial);
    if (it == m_processes.end()) return false;
    return (*it)->state() == QProcess::Running;
}

bool ScrcpyManager::start(const QString &serial, const QStringList &extraArgs) {
    if (isMirroring(serial)) return true;
    if (!isScrcpyInstalled()) {
        m_lastError = "scrcpy is not installed. Install with: sudo apt install scrcpy";
        emit mirroringFailed(serial, m_lastError);
        return false;
    }
    QStringList args = buildArgs(serial, extraArgs);
    spdlog::info("Starting scrcpy: scrcpy {}", args.join(" ").toStdString());
    auto *proc = new QProcess(this);
    // important: connect finished to cleanup
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, serial, proc](int code, QProcess::ExitStatus status){
        Q_UNUSED(status)
        if (code != 0) {
            QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed();
            if (!err.isEmpty()) {
                m_lastError = err;
                emit mirroringFailed(serial, err);
            }
        }
        emit mirroringStopped(serial);
        m_processes.remove(serial);
        proc->deleteLater();
    });
    proc->start("scrcpy", args);
    if (!proc->waitForStarted(3000)) {
        m_lastError = "Failed to start scrcpy";
        emit mirroringFailed(serial, m_lastError);
        proc->deleteLater();
        return false;
    }
    // wait briefly to catch immediate failure
    QThread::msleep(400);
    if (proc->state() != QProcess::Running) {
        m_lastError = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        if (m_lastError.isEmpty()) m_lastError = "scrcpy exited immediately";
        emit mirroringFailed(serial, m_lastError);
        proc->deleteLater();
        return false;
    }
    m_processes[serial] = proc;
    emit mirroringStarted(serial);
    return true;
}

bool ScrcpyManager::stop(const QString &serial) {
    auto it = m_processes.find(serial);
    if (it == m_processes.end()) return false;
    auto *proc = it.value();
    if (proc->state() == QProcess::Running) {
        proc->terminate();
        if (!proc->waitForFinished(3000)) {
            proc->kill();
            proc->waitForFinished(1000);
        }
    }
    m_processes.remove(serial);
    emit mirroringStopped(serial);
    return true;
}

void ScrcpyManager::stopAll() {
    for (auto serial : m_processes.keys()) {
        stop(serial);
    }
}

QString ScrcpyManager::defaultScreenshotPath(const QString &serial) const {
    QString dir = m_settings ? m_settings->recording.outputDir : QDir::homePath() + "/Pictures";
    if (dir.isEmpty()) dir = QDir::homePath() + "/Pictures";
    QDir().mkpath(dir);
    QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return QString("%1/android-control-%2-%3.png").arg(dir, serial, ts);
}

QString ScrcpyManager::defaultRecordPath(const QString &serial) const {
    QString dir = m_settings ? m_settings->recording.outputDir : QDir::homePath() + "/Videos/AndroidControl";
    if (dir.isEmpty()) dir = QDir::homePath() + "/Videos/AndroidControl";
    QDir().mkpath(dir);
    QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString ext = m_settings ? m_settings->recording.videoFormat : "mp4";
    if (ext.isEmpty()) ext = "mp4";
    return QString("%1/record-%2-%3.%4").arg(dir, serial, ts, ext);
}

bool ScrcpyManager::takeScreenshot(const QString &serial, const QString &outputPath) {
    QString path = outputPath.isEmpty() ? defaultScreenshotPath(serial) : outputPath;
    QDir().mkpath(QFileInfo(path).absolutePath());
    QString cmd = QString("adb -s %1 exec-out screencap -p > \"%2\"").arg(serial, path);
    // Use adb directly with redirection via QProcess
    QProcess p;
    p.setStandardOutputFile(path);
    p.start("adb", {"-s", serial, "exec-out", "screencap", "-p"});
    if (!p.waitForFinished(10000)) {
        m_lastError = "Screenshot timed out";
        return false;
    }
    if (p.exitCode() != 0) {
        m_lastError = QString::fromUtf8(p.readAllStandardError());
        return false;
    }
    QFileInfo fi(path);
    if (!fi.exists() || fi.size() == 0) {
        m_lastError = "Screenshot failed - empty file";
        return false;
    }
    emit screenshotTaken(path);
    spdlog::info("Screenshot saved to {}", path.toStdString());
    return true;
}

bool ScrcpyManager::startRecording(const QString &serial, const QString &outputPath) {
    QString path = outputPath.isEmpty() ? defaultRecordPath(serial) : outputPath;
    QDir().mkpath(QFileInfo(path).absolutePath());
    QStringList extra;
    extra << "--record" << path;
    // adjust bitrate per quality
    if (m_settings) {
        if (m_settings->recording.recordingQuality == "low") extra << "--video-bit-rate" << "2000000";
        else if (m_settings->recording.recordingQuality == "medium") extra << "--video-bit-rate" << "4000000";
    }
    if (isMirroring(serial)) stop(serial);
    // small delay
    QThread::msleep(200);
    bool ok = start(serial, extra);
    if (ok) {
        m_isRecording[serial] = true;
        emit recordingStarted(path);
    }
    return ok;
}

bool ScrcpyManager::sendKeyEvent(const QString &serial, int keyCode) {
    QProcess::execute("adb", {"-s", serial, "shell", "input", "keyevent", QString::number(keyCode)});
    return true;
}

bool ScrcpyManager::rotateDevice(const QString &serial) {
    // Cycle user_rotation 0..3
    QProcess p;
    p.start("adb", {"-s", serial, "shell", "settings", "get", "system", "user_rotation"});
    p.waitForFinished(3000);
    QString cur = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    bool ok; int curInt = cur.toInt(&ok);
    if (!ok) curInt = 0;
    int nxt = (curInt + 1) % 4;
    QProcess::execute("adb", {"-s", serial, "shell", "settings", "put", "system", "accelerometer_rotation", "0"});
    QProcess::execute("adb", {"-s", serial, "shell", "settings", "put", "system", "user_rotation", QString::number(nxt)});
    return true;
}

QProcess* ScrcpyManager::getProcess(const QString &serial) const {
    auto it = m_processes.find(serial);
    return it != m_processes.end() ? it.value() : nullptr;
}

} // namespace AndroidControl
