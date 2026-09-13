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
    layout->setContentsMargins(14,10,14,10);
    layout->setSpacing(12);

    m_dot = new QLabel("●", this);
    m_dot->setFixedWidth(14);

    auto *vbox = new QVBoxLayout();
    vbox->setSpacing(2);
    m_nameLabel = new QLabel(this);
    m_nameLabel->setStyleSheet("font-weight: 600; font-size: 14px; color: #f4f6fb;");
    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setStyleSheet("color: #8b93a7; font-size: 11px;");
    vbox->addWidget(m_nameLabel);
    vbox->addWidget(m_subtitleLabel);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("font-size: 11px; font-weight: 600; padding: 4px 10px; border-radius: 10px;");
    m_statusLabel->setAlignment(Qt::AlignCenter);

    m_actionButton = new QPushButton(this);
    m_actionButton->setCursor(Qt::PointingHandCursor);
    m_actionButton->setMinimumWidth(90);
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
            background: #181b24;
            border: 1px solid #2a2e3a;
            border-radius: 12px;
        }
        DeviceWidget:hover { border: 1px solid #3d7eff; }
        DeviceWidget[selected="true"] {
            border: 1px solid #3d7eff;
            background: #1d2333;
        }
    )");
}

void DeviceWidget::updateInfo(const DeviceInfo &info, bool isMirroring, bool isSelected) {
    m_info = info;
    m_isMirroring = isMirroring;
    m_isSelected = isSelected;

    // dot color
    QString color = "#ff5f56";
    if (info.state == DeviceState::Connected) color = "#2ec27e";
    else if (info.state == DeviceState::Unauthorized) color = "#ffbd2e";
    m_dot->setStyleSheet(QString("color: %1; font-size: 16px;").arg(color));

    m_nameLabel->setText(info.displayName());
    QString sub = "USB • Android";
    if (!info.androidVersion.isEmpty()) sub = QString("USB • Android %1").arg(info.androidVersion);
    if (!info.resolution.isEmpty()) sub += " • " + info.resolution;
    if (info.batteryLevel >= 0) sub += QString(" • %1% Battery").arg(info.batteryLevel);
    m_subtitleLabel->setText(sub);

    m_statusLabel->setText(info.statusLabel());
    if (info.state == DeviceState::Connected) {
        m_statusLabel->setStyleSheet("background: rgba(46,194,126,0.15); color: #2ec27e; border-radius: 10px; padding: 4px 10px; font-weight: 600;");
    } else if (info.state == DeviceState::Unauthorized) {
        m_statusLabel->setStyleSheet("background: rgba(255,189,46,0.15); color: #ffbd2e; border-radius: 10px; padding: 4px 10px; font-weight: 600;");
    } else {
        m_statusLabel->setStyleSheet("background: rgba(255,95,86,0.15); color: #ff5f56; border-radius: 10px; padding: 4px 10px; font-weight: 600;");
    }

    m_actionButton->setText(isMirroring ? "■ Stop" : "▶ Mirror");
    if (isMirroring) {
        m_actionButton->setStyleSheet("background: transparent; color: #ff5f56; border: 1px solid #ff5f56; border-radius: 9px; padding: 6px 14px; font-weight: 600;");
        m_actionButton->setProperty("variant", "danger");
    } else {
        m_actionButton->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3d7eff, stop:1 #2f66e0); color: white; border: none; border-radius: 9px; padding: 6px 14px; font-weight: 600;");
        m_actionButton->setProperty("variant", "primary");
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
