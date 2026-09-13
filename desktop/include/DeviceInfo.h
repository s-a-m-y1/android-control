#pragma once
#include <QString>
#include <QMetaType>

namespace AndroidControl {

enum class DeviceState {
    Connected,
    Unauthorized,
    Offline,
    NoPermissions,
    Unknown
};

inline QString deviceStateToString(DeviceState s) {
    switch (s) {
        case DeviceState::Connected: return "Connected";
        case DeviceState::Unauthorized: return "Unauthorized";
        case DeviceState::Offline: return "Offline";
        case DeviceState::NoPermissions: return "No permissions";
        default: return "Unknown";
    }
}

inline DeviceState deviceStateFromString(const QString &str) {
    QString s = str.toLower().trimmed();
    if (s == "device") return DeviceState::Connected;
    if (s == "unauthorized") return DeviceState::Unauthorized;
    if (s == "offline") return DeviceState::Offline;
    if (s.contains("no permissions")) return DeviceState::NoPermissions;
    return DeviceState::Unknown;
}

struct DeviceInfo {
    QString serial;
    DeviceState state = DeviceState::Unknown;
    QString model;
    QString manufacturer;
    QString androidVersion;
    QString apiLevel;
    QString resolution; // e.g. "1080x2400"
    int refreshRate = 0;
    int batteryLevel = -1; // -1 unknown
    QString connectionType = "USB";

    QString displayName() const {
        if (!model.isEmpty()) {
            if (!manufacturer.isEmpty())
                return manufacturer.left(1).toUpper() + manufacturer.mid(1) + " " + model;
            return model;
        }
        return serial;
    }

    bool isConnected() const { return state == DeviceState::Connected; }
    bool isAuthorized() const { return state == DeviceState::Connected; }
    QString statusLabel() const { return deviceStateToString(state); }
};

} // namespace AndroidControl

Q_DECLARE_METATYPE(AndroidControl::DeviceInfo)
