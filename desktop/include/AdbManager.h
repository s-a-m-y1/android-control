#pragma once
#include "DeviceInfo.h"
#include <QObject>
#include <QString>
#include <QTimer>
#include <QProcess>
#include <QFuture>
#include <vector>

namespace AndroidControl {

// All device queries used by the UI run on background threads (via QtConcurrent);
// results are delivered through devicesUpdated so the UI thread never blocks on adb.
class AdbManager : public QObject {
    Q_OBJECT
public:
    explicit AdbManager(QObject *parent = nullptr);
    ~AdbManager() override = default;

    static bool isAdbInstalled();
    static QString adbVersion();

    // Asynchronous listing; emits devicesUpdated when done (on the UI thread).
    void listDevicesAsync(bool detailed = true);
    // Synchronous variants — for tests and non-UI callers only; they block.
    std::vector<DeviceInfo> listDevices(bool detailed = true);
    DeviceInfo getDeviceInfo(const QString &serial);

    bool restartServer();
    bool authorizeDevice(const QString &serial);

    void startAutoRefresh(int intervalMs = 5000);
    void stopAutoRefresh();

signals:
    void devicesUpdated(const std::vector<DeviceInfo> &devices);
    void errorOccurred(const QString &message);
    void adbNotInstalled();

private slots:
    void onAutoRefresh();

private:
    static QString runAdb(const QStringList &args, int timeoutMs = 5000);
    static QString getProp(const QString &serial, const QString &prop);
    static int getBattery(const QString &serial);
    static QString getResolution(const QString &serial);
    static std::vector<DeviceInfo> queryDevices(bool detailed);

    QTimer *m_timer = nullptr;
    bool m_queryRunning = false;
};

} // namespace AndroidControl
