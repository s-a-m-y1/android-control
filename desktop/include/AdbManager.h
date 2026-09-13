#pragma once
#include "DeviceInfo.h"
#include <QObject>
#include <QString>
#include <QTimer>
#include <QProcess>
#include <vector>

namespace AndroidControl {

class AdbManager : public QObject {
    Q_OBJECT
public:
    explicit AdbManager(QObject *parent = nullptr);
    ~AdbManager() override = default;

    static bool isAdbInstalled();
    static QString adbVersion();

    std::vector<DeviceInfo> listDevices(bool detailed = true);
    DeviceInfo getDeviceInfo(const QString &serial);

    bool restartServer();
    bool authorizeDevice(const QString &serial);

    void startAutoRefresh(int intervalMs = 2000);
    void stopAutoRefresh();

signals:
    void devicesUpdated(const std::vector<DeviceInfo> &devices);
    void errorOccurred(const QString &message);
    void adbNotInstalled();

private slots:
    void onAutoRefresh();

private:
    QString runAdb(const QStringList &args, int timeoutMs = 5000);
    QString getProp(const QString &serial, const QString &prop);
    int getBattery(const QString &serial);
    QString getResolution(const QString &serial);
    QString getRefreshRate(const QString &serial);

    QTimer *m_timer = nullptr;
};

} // namespace AndroidControl
