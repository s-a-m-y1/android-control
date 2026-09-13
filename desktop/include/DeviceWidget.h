#pragma once
#include "DeviceInfo.h"
#include <QWidget>
#include <QLabel>
#include <QPushButton>

namespace AndroidControl {

class DeviceWidget : public QWidget {
    Q_OBJECT
public:
    explicit DeviceWidget(const DeviceInfo &info, bool isMirroring, bool isSelected, QWidget *parent = nullptr);
    void updateInfo(const DeviceInfo &info, bool isMirroring, bool isSelected);

signals:
    void selected(const QString &serial);
    void mirrorRequested(const QString &serial);
    void stopRequested(const QString &serial);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void setupUi();

    DeviceInfo m_info;
    bool m_isMirroring = false;
    bool m_isSelected = false;

    QLabel *m_dot = nullptr;
    QLabel *m_nameLabel = nullptr;
    QLabel *m_subtitleLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_actionButton = nullptr;
};

} // namespace AndroidControl
