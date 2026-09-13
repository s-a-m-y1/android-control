#include "DeviceWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QStyle>
#include <QGraphicsDropShadowEffect>

namespace AndroidControl {

DeviceWidget::DeviceWidget(const DeviceInfo &info, bool isMirroring, bool isSelected, QWidget *parent)
    : QWidget(parent), m_info(info), m_isMirroring(isMirroring), m_isSelected(isSelected) {
    setupUi();
    updateInfo(info, isMirroring, isSelected);
    setCursor(Qt::PointingHandCursor);
}

void DeviceWidget::setupUi() {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12,10,12,10);
    layout->setSpacing(12);

    m_dot = new QLabel("●", this);
    m_dot->setFixedWidth(14);
    m_dot->setStyleSheet("font-size: 14px;");

    auto *vbox = new QVBoxLayout();
    vbox->setSpacing(2);
    m_nameLabel = new QLabel(this);
    m_nameLabel->setStyleSheet("font-weight: 600; font-size: 14px;");
    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setStyleSheet("color: #6c757d; font-size: 11px;");
    vbox->addWidget(m_nameLabel);
    vbox->addWidget(m_subtitleLabel);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("font-size: 11px; font-weight: 600; padding: 4px 8px; border-radius: 6px;");
    m_statusLabel->setAlignment(Qt::AlignCenter);

    m_actionButton = new QPushButton(this);
    m_actionButton->setCursor(Qt::PointingHandCursor);
    m_actionButton->setMinimumWidth(80);
    connect(m_actionButton, &QPushButton::clicked, this, [this](){
        if (m_isMirroring) emit stopRequested(m_info.serial);
        else emit mirrorRequested(m_info.serial);
    });

    layout->addWidget(m_dot);
    layout->addLayout(vbox, 1);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_actionButton);

    setStyleSheet(R"(
        DeviceWidget {
            background: palette(base);
            border: 1px solid palette(mid);
            border-radius: 10px;
        }
        DeviceWidget[selected="true"] {
            border: 1px solid #2ec27e;
            background: palette(alternate-base);
        }
    )");
}

void DeviceWidget::updateInfo(const DeviceInfo &info, bool isMirroring, bool isSelected) {
    m_info = info;
    m_isMirroring = isMirroring;
    m_isSelected = isSelected;

    // dot color
    QString color = "#e01b24";
    if (info.state == DeviceState::Connected) color = "#2ec27e";
    else if (info.state == DeviceState::Unauthorized) color = "#ff8c00";
    m_dot->setStyleSheet(QString("color: %1; font-size: 16px;").arg(color));

    m_nameLabel->setText(info.displayName());
    QString sub = "USB • Android";
    if (!info.androidVersion.isEmpty()) sub = QString("USB • Android %1").arg(info.androidVersion);
    if (!info.resolution.isEmpty()) sub += " • " + info.resolution;
    if (info.batteryLevel >= 0) sub += QString(" • %1% Battery").arg(info.batteryLevel);
    m_subtitleLabel->setText(sub);

    m_statusLabel->setText(info.statusLabel());
    if (info.state == DeviceState::Connected) {
        m_statusLabel->setStyleSheet("background: #2ec27e; color: white; border-radius: 6px; padding: 4px 8px; font-weight: 600;");
    } else if (info.state == DeviceState::Unauthorized) {
        m_statusLabel->setStyleSheet("background: #ff8c00; color: white; border-radius: 6px; padding: 4px 8px; font-weight: 600;");
    } else {
        m_statusLabel->setStyleSheet("background: #e01b24; color: white; border-radius: 6px; padding: 4px 8px; font-weight: 600;");
    }

    m_actionButton->setText(isMirroring ? "Stop" : "Mirror");
    if (isMirroring) {
        m_actionButton->setStyleSheet("background: #e01b24; color: white; border-radius: 6px; padding: 6px 12px; font-weight: 600;");
    } else {
        m_actionButton->setStyleSheet("background: #2ec27e; color: white; border-radius: 6px; padding: 6px 12px; font-weight: 600;");
    }
    m_actionButton->setEnabled(info.state == DeviceState::Connected);

    setProperty("selected", isSelected);
    style()->unpolish(this);
    style()->polish(this);
}

void DeviceWidget::mousePressEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    emit selected(m_info.serial);
}

} // namespace AndroidControl
