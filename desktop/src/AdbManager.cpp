#include "AdbManager.h"
#include <QProcess>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>
#include <spdlog/spdlog.h>

namespace AndroidControl {

AdbManager::AdbManager(QObject *parent) : QObject(parent) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AdbManager::onAutoRefresh);
}

bool AdbManager::isAdbInstalled() {
    return !QStandardPaths::findExecutable("adb").isEmpty();
}

QString AdbManager::adbVersion() {
    QProcess p;
    p.start("adb", {"version"});
    if (!p.waitForFinished(3000)) return {};
    QString out = QString::fromUtf8(p.readAllStandardOutput() + p.readAllStandardError());
    QRegularExpression re(R"(version\s+([\d\.]+))");
    auto m = re.match(out);
    if (m.hasMatch()) return m.captured(1);
    return out.split('\n').first().trimmed();
}

QString AdbManager::runAdb(const QStringList &args, int timeoutMs) {
    QProcess p;
    p.start("adb", args);
    if (!p.waitForFinished(timeoutMs)) {
        throw std::runtime_error("adb timeout: adb " + args.join(' ').toStdString());
    }
    if (p.exitCode() != 0) {
        QString err = QString::fromUtf8(p.readAllStandardError()).trimmed();
        if (err.isEmpty()) err = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
        throw std::runtime_error(err.toStdString());
    }
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

QString AdbManager::getProp(const QString &serial, const QString &prop) {
    try {
        return runAdb({"-s", serial, "shell", "getprop", prop}, 4000);
    } catch (...) {
        return {};
    }
}

int AdbManager::getBattery(const QString &serial) {
    try {
        QString out = runAdb({"-s", serial, "shell", "dumpsys", "battery"}, 4000);
        QRegularExpression re(R"(level:\s*(\d+))");
        auto m = re.match(out);
        if (m.hasMatch()) return m.captured(1).toInt();
    } catch (...) {}
    return -1;
}

QString AdbManager::getResolution(const QString &serial) {
    try {
        QString out = runAdb({"-s", serial, "shell", "wm", "size"}, 4000);
        QRegularExpression re(R"((\d+x\d+))");
        auto m = re.match(out);
        if (m.hasMatch()) return m.captured(1);
    } catch (...) {}
    return {};
}

QString AdbManager::getRefreshRate(const QString &serial) {
    // Not critical
    return {};
}

std::vector<DeviceInfo> AdbManager::listDevices(bool detailed) {
    if (!isAdbInstalled()) {
        throw std::runtime_error("ADB is not installed");
    }
    QProcess p;
    p.start("adb", {"devices", "-l"});
    if (!p.waitForFinished(5000)) {
        throw std::runtime_error("adb devices timeout");
    }
    QString out = QString::fromUtf8(p.readAllStandardOutput());
    QString err = QString::fromUtf8(p.readAllStandardError());
    if (p.exitCode() != 0) {
        throw std::runtime_error(err.toStdString());
    }

    std::vector<DeviceInfo> devices;
    QStringList lines = out.split('\n');
    // skip first line
    for (int i = 1; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line.isEmpty()) continue;
        QStringList parts = line.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);
        if (parts.isEmpty()) continue;
        QString serial = parts[0];
        QString stateStr = parts.size() > 1 ? parts[1] : "unknown";
        DeviceInfo info;
        info.serial = serial;
        info.state = deviceStateFromString(stateStr);
        // parse model: from -l extras
        for (const auto &pt : parts) {
            if (pt.startsWith("model:")) {
                info.model = pt.mid(6).replace('_', ' ');
            }
        }
        devices.push_back(info);
    }

    if (detailed) {
        for (auto &d : devices) {
            if (d.state == DeviceState::Connected) {
                d.model = getProp(d.serial, "ro.product.model").isEmpty() ? d.model : getProp(d.serial, "ro.product.model");
                d.manufacturer = getProp(d.serial, "ro.product.manufacturer");
                d.androidVersion = getProp(d.serial, "ro.build.version.release");
                d.apiLevel = getProp(d.serial, "ro.build.version.sdk");
                d.batteryLevel = getBattery(d.serial);
                d.resolution = getResolution(d.serial);
            }
        }
    }
    return devices;
}

DeviceInfo AdbManager::getDeviceInfo(const QString &serial) {
    auto devices = listDevices(true);
    for (auto &d : devices) if (d.serial == serial) return d;
    DeviceInfo info;
    info.serial = serial;
    info.state = DeviceState::Unknown;
    info.model = getProp(serial, "ro.product.model");
    info.manufacturer = getProp(serial, "ro.product.manufacturer");
    info.androidVersion = getProp(serial, "ro.build.version.release");
    info.apiLevel = getProp(serial, "ro.build.version.sdk");
    info.batteryLevel = getBattery(serial);
    info.resolution = getResolution(serial);
    return info;
}

bool AdbManager::restartServer() {
    try {
        QProcess::execute("adb", {"kill-server"});
        QProcess::execute("adb", {"start-server"});
        spdlog::info("ADB server restarted");
        return true;
    } catch (...) {
        return false;
    }
}

bool AdbManager::authorizeDevice(const QString &serial) {
    Q_UNUSED(serial);
    // Just trigger wait-for-device
    try {
        runAdb({"-s", serial, "wait-for-device"}, 10000);
        return true;
    } catch (...) { return false; }
}

void AdbManager::startAutoRefresh(int intervalMs) {
    m_timer->start(intervalMs);
}

void AdbManager::stopAutoRefresh() {
    m_timer->stop();
}

void AdbManager::onAutoRefresh() {
    try {
        auto devs = listDevices(true);
        emit devicesUpdated(devs);
    } catch (const std::exception &e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

} // namespace AndroidControl
