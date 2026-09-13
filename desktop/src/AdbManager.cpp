#include "AdbManager.h"
#include <QProcess>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>

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

std::vector<DeviceInfo> AdbManager::queryDevices(bool detailed) {
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
        // One combined shell call instead of six getprop/dumpsys round trips per device.
        for (auto &d : devices) {
            if (d.state == DeviceState::Connected) {
                try {
                    QString out2 = runAdb({"-s", d.serial, "shell",
                        "getprop ro.product.model; getprop ro.product.manufacturer; "
                        "getprop ro.build.version.release; getprop ro.build.version.sdk; "
                        "dumpsys battery | grep level; wm size"}, 6000);
                    QStringList vals = out2.split('\n', Qt::SkipEmptyParts);
                    auto take = [&](int i) { return i < vals.size() ? vals[i].trimmed() : QString(); };
                    QString model = take(0);
                    if (!model.isEmpty()) d.model = model;
                    d.manufacturer = take(1);
                    d.androidVersion = take(2);
                    d.apiLevel = take(3);
                    static const QRegularExpression reLevel(R"(level:\s*(\d+))");
                    auto mL = reLevel.match(out2);
                    if (mL.hasMatch()) d.batteryLevel = mL.captured(1).toInt();
                    static const QRegularExpression reRes(R"((\d+x\d+))");
                    auto mR = reRes.match(out2);
                    if (mR.hasMatch()) d.resolution = mR.captured(1);
                } catch (...) {}
            }
        }
    }
    return devices;
}

void AdbManager::listDevicesAsync(bool detailed) {
    if (m_queryRunning) return; // don't pile up queries if adb is slow
    m_queryRunning = true;
    auto *self = this;
    (void)QtConcurrent::run([self, detailed]() {
        std::vector<DeviceInfo> devs;
        QString err;
        try {
            devs = queryDevices(detailed);
        } catch (const std::exception &e) {
            err = QString::fromUtf8(e.what());
        }
        QMetaObject::invokeMethod(self, [self, devs, err]() {
            self->m_queryRunning = false;
            if (err.isEmpty()) emit self->devicesUpdated(devs);
            else emit self->errorOccurred(err);
        }, Qt::QueuedConnection);
    });
}

std::vector<DeviceInfo> AdbManager::listDevices(bool detailed) {
    return queryDevices(detailed);
}

DeviceInfo AdbManager::getDeviceInfo(const QString &serial) {
    auto devices = queryDevices(true);
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
    QProcess::execute("adb", {"kill-server"});
    QProcess::execute("adb", {"start-server"});
    qInfo() << "ADB server restarted";
    return true;
}

bool AdbManager::authorizeDevice(const QString &serial) {
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
    listDevicesAsync(true);
}

} // namespace AndroidControl
