#include "MainWindow.h"
#include "DeviceWidget.h"
#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QScrollArea>
#include <QFrame>
#include <QGroupBox>
#include <QGridLayout>
#include <QFormLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QTimer>
#include <QDebug>
#include <spdlog/spdlog.h>

namespace AndroidControl {

MainWindow::MainWindow(AdbManager *adb, ScrcpyManager *scrcpy, SettingsManager *settings,
                       FileTransferManager *fileMgr, ClipboardManager *clipboard, QWidget *parent)
    : QMainWindow(parent), m_adb(adb), m_scrcpy(scrcpy), m_settings(settings),
      m_fileMgr(fileMgr), m_clipboard(clipboard) {

    setWindowTitle("Android Control — USB • ADB • scrcpy");
    setMinimumSize(860, 700);
    resize(960, 800);

    setupUi();
    setupMenu();
    checkDependencies();

    connect(m_adb, &AdbManager::devicesUpdated, this, &MainWindow::onDevicesUpdated);
    connect(m_adb, &AdbManager::errorOccurred, this, &MainWindow::onError);
    connect(m_scrcpy, &ScrcpyManager::mirroringStarted, this, &MainWindow::onMirroringStarted);
    connect(m_scrcpy, &ScrcpyManager::mirroringFailed, this, &MainWindow::onMirroringFailed);
    connect(m_scrcpy, &ScrcpyManager::screenshotTaken, this, [this](const QString &path){
        statusBar()->showMessage("Screenshot saved: " + path, 5000);
    });

    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::refreshDevices);
    m_refreshTimer->start(2000);

    // Restore selected device
    m_selectedSerial = m_settings->connection.selectedDevice;
    refreshDevices();
    updatePreview();
    updateStatusBar();
}

void MainWindow::setupUi() {
    m_central = new QWidget(this);
    setCentralWidget(m_central);
    m_mainLayout = new QVBoxLayout(m_central);
    m_mainLayout->setContentsMargins(16,12,16,12);
    m_mainLayout->setSpacing(12);

    // Banner for ADB missing
    m_banner = new QWidget(this);
    m_banner->setStyleSheet("background: #fff3cd; border: 1px solid #ffe69c; border-radius: 8px; padding: 8px;");
    auto *bannerLayout = new QVBoxLayout(m_banner);
    m_bannerLabel = new QLabel(m_banner);
    m_bannerLabel->setWordWrap(true);
    m_bannerLabel->setStyleSheet("color: #664d03;");
    bannerLayout->addWidget(m_bannerLabel);
    m_banner->setVisible(false);
    m_mainLayout->addWidget(m_banner);

    // Header: title + refresh
    auto *header = new QHBoxLayout();
    auto *title = new QLabel("Devices", this);
    title->setStyleSheet("font-size: 18px; font-weight: 700;");
    m_deviceCountLabel = new QLabel("", this);
    m_deviceCountLabel->setStyleSheet("color: #6c757d; font-size: 12px;");
    auto *refreshBtn = new QPushButton("Refresh", this);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshDevices);
    header->addWidget(title);
    header->addWidget(m_deviceCountLabel);
    header->addStretch();
    header->addWidget(refreshBtn);
    m_mainLayout->addLayout(header);

    // Device list container with scroll
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setMaximumHeight(220);
    m_deviceListContainer = new QWidget(scroll);
    m_deviceListLayout = new QVBoxLayout(m_deviceListContainer);
    m_deviceListLayout->setContentsMargins(0,0,0,0);
    m_deviceListLayout->setSpacing(8);
    m_deviceListContainer->setLayout(m_deviceListLayout);
    scroll->setWidget(m_deviceListContainer);
    m_mainLayout->addWidget(scroll);

    // Preview frame (phone screen placeholder)
    m_previewFrame = new QFrame(this);
    m_previewFrame->setFrameShape(QFrame::StyledPanel);
    m_previewFrame->setStyleSheet(R"(
        QFrame { background: #0f0f0f; border-radius: 18px; border: 1px solid #2d2d2d; }
        QLabel { color: white; }
        QPushButton { border-radius: 8px; padding: 8px 16px; font-weight: 600; }
    )");
    m_previewFrame->setMinimumHeight(340);
    m_previewFrame->setMaximumWidth(480);
    auto *previewLayout = new QVBoxLayout(m_previewFrame);
    previewLayout->setAlignment(Qt::AlignCenter);
    previewLayout->setSpacing(8);
    previewLayout->setContentsMargins(24,24,24,24);

    m_previewIcon = new QLabel("📱", m_previewFrame);
    m_previewIcon->setAlignment(Qt::AlignCenter);
    m_previewIcon->setStyleSheet("font-size: 48px;");
    m_previewTitle = new QLabel("No device selected", m_previewFrame);
    m_previewTitle->setAlignment(Qt::AlignCenter);
    m_previewTitle->setStyleSheet("font-size: 16px; font-weight: 700;");
    m_previewSubtitle = new QLabel("Select a device to start mirroring", m_previewFrame);
    m_previewSubtitle->setAlignment(Qt::AlignCenter);
    m_previewSubtitle->setWordWrap(true);
    m_previewSubtitle->setStyleSheet("color: #adb5bd; font-size: 12px;");
    m_previewHint = new QLabel("Mouse: left=touch • right=Back • middle=Home • wheel=scroll\nDrag & copy/paste via scrcpy", m_previewFrame);
    m_previewHint->setAlignment(Qt::AlignCenter);
    m_previewHint->setWordWrap(true);
    m_previewHint->setStyleSheet("color: #6c757d; font-size: 11px;");
    m_centerMirrorButton = new QPushButton("Start Mirroring", m_previewFrame);
    m_centerMirrorButton->setCursor(Qt::PointingHandCursor);
    m_centerMirrorButton->setStyleSheet("background: #2ec27e; color: white; padding: 10px 24px; font-weight: 700; border-radius: 8px;");
    connect(m_centerMirrorButton, &QPushButton::clicked, this, [this](){
        if (m_selectedSerial.isEmpty()) {
            QMessageBox::information(this, "No device", "Select a device first.");
            return;
        }
        onMirrorRequested(m_selectedSerial);
    });

    previewLayout->addWidget(m_previewIcon);
    previewLayout->addWidget(m_previewTitle);
    previewLayout->addWidget(m_previewSubtitle);
    previewLayout->addWidget(m_previewHint);
    previewLayout->addWidget(m_centerMirrorButton, 0, Qt::AlignHCenter);

    auto *previewWrapper = new QHBoxLayout();
    previewWrapper->addStretch();
    previewWrapper->addWidget(m_previewFrame);
    previewWrapper->addStretch();
    m_mainLayout->addLayout(previewWrapper);

    // Info group
    m_infoGroup = new QGroupBox("Device Information", this);
    auto *infoLayout = new QVBoxLayout(m_infoGroup);
    m_infoLabel = new QLabel("Select a device", m_infoGroup);
    m_infoLabel->setStyleSheet("color: #495057; font-size: 12px;");
    m_infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_infoLabel->setWordWrap(true);
    infoLayout->addWidget(m_infoLabel);
    m_infoGroup->setVisible(false);
    m_mainLayout->addWidget(m_infoGroup);

    // Status bar
    auto *statusRow = new QHBoxLayout();
    m_statusDot = new QLabel("●", this);
    m_statusDot->setStyleSheet("color: #e01b24; font-size: 14px;");
    m_statusLabel = new QLabel("Disconnected", this);
    m_statusLabel->setStyleSheet("font-size: 12px; font-weight: 600;");
    m_fpsLabel = new QLabel(QString::number(m_settings->display.maxFps) + " FPS", this);
    m_fpsLabel->setStyleSheet("color: #6c757d; font-size: 11px; border: 1px solid #dee2e6; border-radius: 6px; padding: 2px 6px;");
    m_usbLabel = new QLabel("USB", this);
    m_usbLabel->setStyleSheet("color: #6c757d; font-size: 11px; border: 1px solid #dee2e6; border-radius: 6px; padding: 2px 6px; background: palette(base);");
    statusRow->addWidget(m_statusDot);
    statusRow->addWidget(m_statusLabel);
    statusRow->addStretch();
    statusRow->addWidget(m_fpsLabel);
    statusRow->addWidget(m_usbLabel);
    m_mainLayout->addLayout(statusRow);

    // Action buttons row 1: main controls
    auto *actions = new QHBoxLayout();
    actions->setSpacing(8);
    m_btnScreenshot = new QPushButton("Screenshot", this);
    m_btnRecord = new QPushButton("Record", this);
    m_btnRotate = new QPushButton("Rotate", this);
    m_btnFullscreen = new QPushButton("Fullscreen", this);
    m_btnDisconnect = new QPushButton("Disconnect", this);
    for (auto *b : {m_btnScreenshot, m_btnRecord, m_btnRotate, m_btnFullscreen, m_btnDisconnect}) {
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet("QPushButton { background: palette(button); border: 1px solid palette(mid); border-radius: 8px; padding: 8px 14px; } QPushButton:hover { background: palette(midlight); }");
        actions->addWidget(b);
    }
    m_btnDisconnect->setStyleSheet("QPushButton { background: #e01b24; color: white; border-radius: 8px; padding: 8px 14px; font-weight: 600; }");
    connect(m_btnScreenshot, &QPushButton::clicked, this, &MainWindow::onScreenshot);
    connect(m_btnRecord, &QPushButton::clicked, this, &MainWindow::onRecord);
    connect(m_btnRotate, &QPushButton::clicked, this, &MainWindow::onRotate);
    connect(m_btnFullscreen, &QPushButton::clicked, this, &MainWindow::onFullscreen);
    connect(m_btnDisconnect, &QPushButton::clicked, this, &MainWindow::onDisconnect);
    m_mainLayout->addLayout(actions);

    // Row 2: Android buttons + file transfer + clipboard
    auto *actions2 = new QHBoxLayout();
    actions2->setSpacing(8);
    m_btnBack = new QPushButton("Back", this);
    m_btnHome = new QPushButton("Home", this);
    m_btnRecent = new QPushButton("Recent", this);
    m_btnPush = new QPushButton("Push File → Device", this);
    m_btnPull = new QPushButton("Pull File ← Device", this);
    auto *btnCopy = new QPushButton("Copy", this);
    auto *btnPaste = new QPushButton("Paste", this);
    for (auto *b : {m_btnBack, m_btnHome, m_btnRecent, m_btnPush, m_btnPull, btnCopy, btnPaste}) {
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet("QPushButton { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; padding: 6px 10px; font-size: 11px; }");
        actions2->addWidget(b);
    }
    connect(m_btnBack, &QPushButton::clicked, this, [this](){
        if (!m_selectedSerial.isEmpty()) m_scrcpy->sendKeyEvent(m_selectedSerial, 4); // BACK
    });
    connect(m_btnHome, &QPushButton::clicked, this, [this](){
        if (!m_selectedSerial.isEmpty()) m_scrcpy->sendKeyEvent(m_selectedSerial, 3); // HOME
    });
    connect(m_btnRecent, &QPushButton::clicked, this, [this](){
        if (!m_selectedSerial.isEmpty()) m_scrcpy->sendKeyEvent(m_selectedSerial, 187); // APP_SWITCH
    });
    connect(m_btnPush, &QPushButton::clicked, this, &MainWindow::onPushFile);
    connect(m_btnPull, &QPushButton::clicked, this, &MainWindow::onPullFile);
    connect(btnCopy, &QPushButton::clicked, this, &MainWindow::onCopyClipboard);
    connect(btnPaste, &QPushButton::clicked, this, &MainWindow::onPasteClipboard);
    m_mainLayout->addLayout(actions2);

    // Footer shortcuts hint
    auto *hint = new QLabel("Shortcuts: Ctrl+R Refresh • Ctrl+, Settings • Ctrl+S Screenshot • Ctrl+Shift+R Record • Alt+Enter Fullscreen", this);
    hint->setStyleSheet("color: #6c757d; font-size: 10px;");
    hint->setAlignment(Qt::AlignCenter);
    hint->setWordWrap(true);
    m_mainLayout->addWidget(hint);
    m_mainLayout->addStretch();

    // Status bar
    statusBar()->showMessage("Ready");
}

void MainWindow::setupMenu() {
    auto *menu = menuBar()->addMenu("File");
    menu->addAction("Refresh Devices", this, &MainWindow::refreshDevices, QKeySequence("Ctrl+R"));
    menu->addAction("Settings", this, &MainWindow::onSettings, QKeySequence("Ctrl+,"));
    menu->addSeparator();
    menu->addAction("Quit", qApp, &QApplication::quit, QKeySequence("Ctrl+Q"));

    auto *help = menuBar()->addMenu("Help");
    help->addAction("About", this, [this](){
        QMessageBox::about(this, "Android Control",
            "<b>Android Control 1.0.0</b><br>"
            "Modern Android screen mirroring for Ubuntu.<br>"
            "Built with Qt6, scrcpy & ADB.<br><br>"
            "© 2026 Android Control • Hardware-accelerated • Low latency");
    });
    help->addAction("Install Guide", this, &MainWindow::showAdbGuide);

    auto *toolbar = addToolBar("Main");
    toolbar->addAction("Settings", this, &MainWindow::onSettings);
    toolbar->addAction("Refresh", this, &MainWindow::refreshDevices);
}

void MainWindow::checkDependencies() {
    if (!AdbManager::isAdbInstalled()) {
        m_banner->setVisible(true);
        m_bannerLabel->setText("<b>ADB is not installed.</b> Install Android Platform Tools to connect devices.<br><code>sudo apt update && sudo apt install android-sdk-platform-tools scrcpy</code>");
    } else if (!ScrcpyManager::isScrcpyInstalled()) {
        m_banner->setVisible(true);
        m_bannerLabel->setText("<b>scrcpy is not installed.</b> Screen mirroring requires scrcpy.<br><code>sudo apt install scrcpy</code>");
    } else {
        m_banner->setVisible(false);
    }
}

void MainWindow::showAdbGuide() {
    QMessageBox::information(this, "Install ADB & scrcpy",
        "Run in Terminal:\n\n"
        "sudo apt update\n"
        "sudo apt install android-sdk-platform-tools scrcpy\n\n"
        "Enable USB Debugging:\n"
        "1. Settings → About phone → tap Build number 7 times\n"
        "2. Settings → Developer options → enable USB Debugging\n"
        "3. Connect via USB and tap 'Allow' on the phone");
}

void MainWindow::refreshDevices() {
    try {
        auto devs = m_adb->listDevices(true);
        onDevicesUpdated(devs);
    } catch (const std::exception &e) {
        onError(QString::fromStdString(e.what()));
    }
}

void MainWindow::onDevicesUpdated(const std::vector<DeviceInfo> &devices) {
    m_devices = devices;
    m_deviceCountLabel->setText(QString("%1 device(s)").arg(devices.size()));
    rebuildDeviceList();
    updatePreview();
    updateStatusBar();
}

void MainWindow::rebuildDeviceList() {
    // clear
    QLayoutItem *child;
    while ((child = m_deviceListLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    if (m_devices.empty()) {
        auto *empty = new QLabel("No Android device detected\n\nConnect your phone using USB\nand enable USB Debugging.", this);
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet("color: #6c757d; padding: 16px; border: 1px dashed #dee2e6; border-radius: 8px; background: palette(base);");
        m_deviceListLayout->addWidget(empty);
        auto *btn = new QPushButton("Refresh Devices", this);
        btn->setStyleSheet("background: #2ec27e; color: white; border-radius: 8px; padding: 8px;");
        connect(btn, &QPushButton::clicked, this, &MainWindow::refreshDevices);
        m_deviceListLayout->addWidget(btn, 0, Qt::AlignHCenter);
        m_selectedSerial.clear();
        m_infoGroup->setVisible(false);
        return;
    }
    // auto-select if needed
    QStringList serials;
    for (auto &d : m_devices) serials << d.serial;
    if (m_selectedSerial.isEmpty() || !serials.contains(m_selectedSerial)) {
        // prefer connected
        for (auto &d : m_devices) if (d.state == DeviceState::Connected) { m_selectedSerial = d.serial; break; }
        if (m_selectedSerial.isEmpty()) m_selectedSerial = m_devices[0].serial;
        m_settings->connection.selectedDevice = m_selectedSerial;
        m_settings->save();
    }
    for (auto &d : m_devices) {
        bool mirroring = m_scrcpy->isMirroring(d.serial);
        bool selected = d.serial == m_selectedSerial;
        auto *w = new DeviceWidget(d, mirroring, selected, this);
        connect(w, &DeviceWidget::selected, this, &MainWindow::onDeviceSelected);
        connect(w, &DeviceWidget::mirrorRequested, this, &MainWindow::onMirrorRequested);
        connect(w, &DeviceWidget::stopRequested, this, [this](const QString &s){ m_scrcpy->stop(s); refreshDevices(); });
        m_deviceListLayout->addWidget(w);
    }
    m_deviceListLayout->addStretch();

    // update info
    auto *sel = findSelectedDevice();
    if (sel && sel->state == DeviceState::Connected) {
        m_infoGroup->setVisible(true);
        QString info = QString(
            "<b>Serial:</b> %1<br>"
            "<b>Model:</b> %2<br>"
            "<b>Manufacturer:</b> %3<br>"
            "<b>Android:</b> %4 (SDK %5)<br>"
            "<b>Resolution:</b> %6<br>"
            "<b>Battery:</b> %7%<br>"
            "<b>Connection:</b> %8"
        ).arg(sel->serial, sel->model.isEmpty()?"—":sel->model, sel->manufacturer.isEmpty()?"—":sel->manufacturer,
              sel->androidVersion.isEmpty()?"—":sel->androidVersion, sel->apiLevel.isEmpty()?"—":sel->apiLevel,
              sel->resolution.isEmpty()?"—":sel->resolution,
              sel->batteryLevel<0?"—":QString::number(sel->batteryLevel), sel->connectionType);
        m_infoLabel->setText(info);
    } else {
        m_infoGroup->setVisible(false);
    }
}

DeviceInfo* MainWindow::findSelectedDevice() {
    for (auto &d : m_devices) if (d.serial == m_selectedSerial) return &d;
    return nullptr;
}

void MainWindow::onDeviceSelected(const QString &serial) {
    m_selectedSerial = serial;
    m_settings->connection.selectedDevice = serial;
    m_settings->save();
    rebuildDeviceList();
    updatePreview();
    updateStatusBar();
}

void MainWindow::onMirrorRequested(const QString &serial) {
    if (m_scrcpy->isMirroring(serial)) {
        m_scrcpy->stop(serial);
        statusBar()->showMessage("Stopped mirroring " + serial, 3000);
        refreshDevices();
        updatePreview();
        return;
    }
    // check state
    auto *dev = findSelectedDevice();
    // find requested dev
    const DeviceInfo *req = nullptr;
    for (auto &d : m_devices) if (d.serial == serial) req = &d;
    if (req && req->state == DeviceState::Unauthorized) {
        showUnauthorizedDialog(*req);
        return;
    }
    if (req && req->state != DeviceState::Connected) {
        QMessageBox::warning(this, "Device not ready", "Device is " + req->statusLabel());
        return;
    }
    if (!ScrcpyManager::isScrcpyInstalled()) {
        showAdbGuide();
        return;
    }
    statusBar()->showMessage("Starting mirroring for " + serial + "...");
    bool ok = m_scrcpy->start(serial);
    if (!ok) {
        QMessageBox::critical(this, "Mirroring failed", m_scrcpy->lastError());
    } else {
        m_selectedSerial = serial;
        refreshDevices();
    }
}

void MainWindow::showUnauthorizedDialog(const DeviceInfo &info) {
    QMessageBox::information(this, "Authorize USB Debugging",
        QString("Device %1 is unauthorized.\n\nOn your phone you should see:\n'Allow USB debugging?'\n\nTap 'Allow' to authorize this computer.\n\nIf you don't see the prompt, disconnect and reconnect USB, ensure USB Debugging is enabled.").arg(info.serial));
}

void MainWindow::updatePreview() {
    if (m_selectedSerial.isEmpty()) {
        m_previewTitle->setText("No device selected");
        m_previewSubtitle->setText("Select a device to start mirroring");
        m_previewIcon->setText("📱");
        m_centerMirrorButton->setText("Start Mirroring");
        m_centerMirrorButton->setStyleSheet("background: #2ec27e; color: white; padding: 10px 24px; font-weight: 700; border-radius: 8px;");
        return;
    }
    auto *dev = findSelectedDevice();
    if (!dev) return;
    bool mirroring = m_scrcpy->isMirroring(m_selectedSerial);
    if (mirroring) {
        m_previewTitle->setText("Mirroring active");
        m_previewSubtitle->setText(QString("Mirroring %1 in external hardware-accelerated window.\nControls: left=touch, right=Back, middle=Home, wheel=scroll").arg(dev->displayName()));
        m_previewIcon->setText("🖥️");
        m_centerMirrorButton->setText("Stop Mirroring");
        m_centerMirrorButton->setStyleSheet("background: #e01b24; color: white; padding: 10px 24px; font-weight: 700; border-radius: 8px;");
    } else if (dev->state == DeviceState::Connected) {
        m_previewTitle->setText(dev->displayName());
        QString sub = QString("Ready to mirror");
        if (!dev->resolution.isEmpty()) sub += " • " + dev->resolution;
        if (!dev->androidVersion.isEmpty()) sub += " • Android " + dev->androidVersion;
        if (dev->batteryLevel >= 0) sub += QString(" • Battery %1%").arg(dev->batteryLevel);
        m_previewSubtitle->setText(sub);
        m_previewIcon->setText("📱");
        m_centerMirrorButton->setText("Start Mirroring");
        m_centerMirrorButton->setStyleSheet("background: #2ec27e; color: white; padding: 10px 24px; font-weight: 700; border-radius: 8px;");
    } else if (dev->state == DeviceState::Unauthorized) {
        m_previewTitle->setText("Unauthorized device");
        m_previewSubtitle->setText("Tap 'Allow USB debugging' on your phone to authorize.");
        m_previewIcon->setText("⚠️");
        m_centerMirrorButton->setText("Refresh");
    } else {
        m_previewTitle->setText(dev->statusLabel());
        m_previewSubtitle->setText(QString("Device %1 is %2. Reconnect USB.").arg(dev->serial, dev->statusLabel()));
        m_previewIcon->setText("❌");
        m_centerMirrorButton->setText("Refresh");
    }
}

void MainWindow::updateStatusBar() {
    if (m_devices.empty()) {
        m_statusDot->setStyleSheet("color: #e01b24; font-size: 14px;");
        m_statusLabel->setText("No device");
        return;
    }
    auto *dev = findSelectedDevice();
    if (!dev) dev = &m_devices[0];
    bool mirroring = m_scrcpy->isMirroring(dev->serial);
    if (mirroring) {
        m_statusDot->setStyleSheet("color: #2ec27e; font-size: 14px;");
        m_statusLabel->setText("Connected • Mirroring");
    } else if (dev->state == DeviceState::Connected) {
        m_statusDot->setStyleSheet("color: #2ec27e; font-size: 14px;");
        m_statusLabel->setText("Connected");
    } else if (dev->state == DeviceState::Unauthorized) {
        m_statusDot->setStyleSheet("color: #ff8c00; font-size: 14px;");
        m_statusLabel->setText("Unauthorized");
    } else {
        m_statusDot->setStyleSheet("color: #e01b24; font-size: 14px;");
        m_statusLabel->setText(dev->statusLabel());
    }
    m_fpsLabel->setText(QString::number(m_settings->display.maxFps) + " FPS");
}

void MainWindow::onMirroringStarted(const QString &serial) {
    Q_UNUSED(serial);
    statusBar()->showMessage("Mirroring started — scrcpy window opened for " + serial, 4000);
    refreshDevices();
    updatePreview();
    updateStatusBar();
}

void MainWindow::onMirroringFailed(const QString &serial, const QString &error) {
    QString friendly = error;
    if (error.contains("unauthorized", Qt::CaseInsensitive)) friendly = "Device unauthorized. Tap 'Allow' on the phone.";
    else if (error.contains("offline", Qt::CaseInsensitive)) friendly = "Device offline. Reconnect USB.";
    QMessageBox::critical(this, "Mirroring failed", friendly + "\n\nRaw: " + error);
    statusBar()->showMessage("Mirroring failed for " + serial, 4000);
}

void MainWindow::onScreenshot() {
    if (m_selectedSerial.isEmpty()) { QMessageBox::information(this, "No device", "Select a device first."); return; }
    auto *dev = findSelectedDevice();
    if (!dev || dev->state != DeviceState::Connected) { QMessageBox::warning(this, "Not connected", "Device not connected."); return; }
    statusBar()->showMessage("Taking screenshot...");
    bool ok = m_scrcpy->takeScreenshot(m_selectedSerial);
    if (!ok) QMessageBox::critical(this, "Screenshot failed", m_scrcpy->lastError());
    else statusBar()->showMessage("Screenshot saved to " + m_settings->recording.outputDir, 4000);
}

void MainWindow::onRecord() {
    if (m_selectedSerial.isEmpty()) { QMessageBox::information(this, "No device", "Select a device."); return; }
    auto *dev = findSelectedDevice();
    if (!dev || dev->state != DeviceState::Connected) { QMessageBox::warning(this, "Not connected", "Device not connected."); return; }
    if (m_isRecording) {
        m_scrcpy->stop(m_selectedSerial);
        m_isRecording = false;
        m_btnRecord->setText("Record");
        statusBar()->showMessage("Recording stopped", 3000);
        return;
    }
    statusBar()->showMessage("Starting recording...");
    bool ok = m_scrcpy->startRecording(m_selectedSerial);
    if (!ok) QMessageBox::critical(this, "Record failed", m_scrcpy->lastError());
    else {
        m_isRecording = true;
        m_btnRecord->setText("Stop Record");
        statusBar()->showMessage("Recording — mirroring window is recording", 4000);
    }
}

void MainWindow::onRotate() {
    if (m_selectedSerial.isEmpty()) { QMessageBox::information(this, "No device", "Select a device."); return; }
    bool ok = m_scrcpy->rotateDevice(m_selectedSerial);
    statusBar()->showMessage(ok ? "Rotation toggled" : "Rotate failed", 3000);
}

void MainWindow::onFullscreen() {
    if (m_selectedSerial.isEmpty()) {
        m_settings->display.fullscreen = !m_settings->display.fullscreen;
        m_settings->save();
        statusBar()->showMessage(QString("Fullscreen %1 for next mirroring").arg(m_settings->display.fullscreen ? "enabled" : "disabled"), 3000);
        return;
    }
    m_settings->display.fullscreen = !m_settings->display.fullscreen;
    m_settings->save();
    statusBar()->showMessage(QString("Fullscreen %1 — restart mirroring to apply").arg(m_settings->display.fullscreen ? "enabled" : "disabled"), 4000);
    if (m_scrcpy->isMirroring(m_selectedSerial)) {
        QString s = m_selectedSerial;
        m_scrcpy->stop(s);
        QTimer::singleShot(500, this, [this, s](){ m_scrcpy->start(s); refreshDevices(); });
    }
}

void MainWindow::onDisconnect() {
    if (m_selectedSerial.isEmpty()) { QMessageBox::information(this, "No device", "Select a device."); return; }
    if (m_scrcpy->isMirroring(m_selectedSerial)) {
        m_scrcpy->stop(m_selectedSerial);
        statusBar()->showMessage("Disconnected " + m_selectedSerial, 3000);
        refreshDevices(); updatePreview(); updateStatusBar();
    } else {
        QMessageBox::information(this, "Not mirroring", "Not mirroring — unplug USB to fully disconnect.");
    }
}

void MainWindow::onPushFile() {
    if (m_selectedSerial.isEmpty()) { QMessageBox::information(this, "No device", "Select a device."); return; }
    QString local = QFileDialog::getOpenFileName(this, "Select file to push to device");
    if (local.isEmpty()) return;
    QString remote = QInputDialog::getText(this, "Remote path", "Device path (e.g. /sdcard/Download/)", QLineEdit::Normal, "/sdcard/Download/" + QFileInfo(local).fileName());
    if (remote.isEmpty()) return;
    // check overwrite
    statusBar()->showMessage("Pushing file...");
    bool ok = m_fileMgr->pushFile(m_selectedSerial, local, remote);
    statusBar()->showMessage(ok ? "Push completed" : "Push failed", 4000);
    if (!ok) QMessageBox::critical(this, "Push failed", "Check error and that remote path exists.");
}

void MainWindow::onPullFile() {
    if (m_selectedSerial.isEmpty()) { QMessageBox::information(this, "No device", "Select a device."); return; }
    QString remote = QInputDialog::getText(this, "Remote path", "Device file path", QLineEdit::Normal, "/sdcard/Download/");
    if (remote.isEmpty()) return;
    QString localDir = QFileDialog::getExistingDirectory(this, "Select local destination");
    if (localDir.isEmpty()) return;
    QString local = localDir + "/" + QFileInfo(remote).fileName();
    // avoid overwrite without prompt
    if (QFileInfo::exists(local)) {
        auto ans = QMessageBox::question(this, "Overwrite?", QString("File %1 exists. Overwrite?").arg(local));
        if (ans != QMessageBox::Yes) return;
    }
    statusBar()->showMessage("Pulling file...");
    bool ok = m_fileMgr->pullFile(m_selectedSerial, remote, local);
    statusBar()->showMessage(ok ? "Pull completed: " + local : "Pull failed", 4000);
}

void MainWindow::onCopyClipboard() {
    if (m_selectedSerial.isEmpty()) { QMessageBox::information(this, "No device", "Select a device."); return; }
    QString text = QInputDialog::getText(this, "Copy to device", "Text to copy");
    if (text.isEmpty()) return;
    bool ok = m_clipboard->copyToDevice(m_selectedSerial, text);
    statusBar()->showMessage(ok ? "Copied to device" : "Copy failed", 3000);
}

void MainWindow::onPasteClipboard() {
    if (m_selectedSerial.isEmpty()) { QMessageBox::information(this, "No device", "Select a device."); return; }
    QString text = m_clipboard->pasteFromDevice(m_selectedSerial);
    if (text.isEmpty()) {
        QMessageBox::information(this, "Clipboard", "Clipboard empty or not accessible.\nNote: Clipboard sync works best when scrcpy mirroring is active with --clipboard-autosync.");
        return;
    }
    QClipboard *cb = QApplication::clipboard();
    cb->setText(text);
    QMessageBox::information(this, "Clipboard pasted", "Text from device (also copied to PC clipboard):\n\n" + text.left(500));
}

void MainWindow::onSettings() {
    // Simple dialog for settings
    QDialog dlg(this);
    dlg.setWindowTitle("Settings");
    dlg.resize(520, 560);
    auto *layout = new QVBoxLayout(&dlg);
    auto *tabs = new QTabWidget(&dlg);

    // Display tab
    auto *dispTab = new QWidget();
    auto *dispForm = new QFormLayout(dispTab);
    auto *spinSize = new QSpinBox(dispTab); spinSize->setRange(0, 4096); spinSize->setValue(m_settings->display.maxSize); spinSize->setSpecialValueText("0 (original)");
    auto *spinFps = new QSpinBox(dispTab); spinFps->setRange(1, 60); spinFps->setValue(m_settings->display.maxFps);
    auto *spinBitrate = new QSpinBox(dispTab); spinBitrate->setRange(500, 50000); spinBitrate->setValue(m_settings->display.videoBitrate/1000); spinBitrate->setSuffix(" kbps");
    auto *comboCodec = new QComboBox(dispTab); comboCodec->addItems({"h264","h265","av1"}); comboCodec->setCurrentText(m_settings->display.videoCodec);
    auto *chkFullscreen = new QCheckBox("Fullscreen", dispTab); chkFullscreen->setChecked(m_settings->display.fullscreen);
    auto *chkStayAwake = new QCheckBox("Keep screen awake", dispTab); chkStayAwake->setChecked(m_settings->display.stayAwake);
    auto *chkShowTouches = new QCheckBox("Show touches", dispTab); chkShowTouches->setChecked(m_settings->display.showTouches);
    auto *chkDisableSaver = new QCheckBox("Disable screensaver", dispTab); chkDisableSaver->setChecked(m_settings->display.disableScreensaver);
    dispForm->addRow("Max size:", spinSize);
    dispForm->addRow("Max FPS:", spinFps);
    dispForm->addRow("Bitrate:", spinBitrate);
    dispForm->addRow("Codec:", comboCodec);
    dispForm->addRow(chkFullscreen);
    dispForm->addRow(chkStayAwake);
    dispForm->addRow(chkShowTouches);
    dispForm->addRow(chkDisableSaver);
    tabs->addTab(dispTab, "Display");

    // Input tab
    auto *inputTab = new QWidget();
    auto *inputForm = new QFormLayout(inputTab);
    auto *chkMouse = new QCheckBox("Mouse control", inputTab); chkMouse->setChecked(m_settings->input.mouseControl);
    auto *chkKeyboard = new QCheckBox("Keyboard control", inputTab); chkKeyboard->setChecked(m_settings->input.keyboardControl);
    auto *chkClipboard = new QCheckBox("Clipboard autosync", inputTab); chkClipboard->setChecked(m_settings->input.clipboardAutosync);
    auto *chkTurnOff = new QCheckBox("Turn screen off while mirroring", inputTab); chkTurnOff->setChecked(m_settings->input.turnScreenOff);
    inputForm->addRow(chkMouse);
    inputForm->addRow(chkKeyboard);
    inputForm->addRow(chkClipboard);
    inputForm->addRow(chkTurnOff);
    auto *hint = new QLabel("Mouse: left=touch, right=Back, middle=Home, wheel=scroll, drag=swipe", inputTab);
    hint->setWordWrap(true); hint->setStyleSheet("color: #6c757d;");
    inputForm->addRow(hint);
    tabs->addTab(inputTab, "Input");

    // Connection tab
    auto *connTab = new QWidget();
    auto *connForm = new QFormLayout(connTab);
    auto *chkAuto = new QCheckBox("Auto reconnect", connTab); chkAuto->setChecked(m_settings->connection.autoReconnect);
    auto *adbLabel = new QLabel(AdbManager::isAdbInstalled() ? AdbManager::adbVersion() : "ADB not installed", connTab);
    auto *btnRestart = new QPushButton("Restart ADB server", connTab);
    connect(btnRestart, &QPushButton::clicked, this, [this, adbLabel](){
        m_adb->restartServer();
        adbLabel->setText(AdbManager::adbVersion());
    });
    connForm->addRow(chkAuto);
    connForm->addRow("ADB version:", adbLabel);
    connForm->addRow(btnRestart);
    tabs->addTab(connTab, "Connection");

    // Recording tab
    auto *recTab = new QWidget();
    auto *recForm = new QFormLayout(recTab);
    auto *editOutDir = new QLineEdit(m_settings->recording.outputDir, recTab);
    auto *btnChoose = new QPushButton("Choose…", recTab);
    connect(btnChoose, &QPushButton::clicked, this, [editOutDir,this](){
        QString dir = QFileDialog::getExistingDirectory(this, "Output directory", editOutDir->text());
        if (!dir.isEmpty()) editOutDir->setText(dir);
    });
    auto *hbox = new QHBoxLayout(); hbox->addWidget(editOutDir,1); hbox->addWidget(btnChoose);
    auto *comboFmt = new QComboBox(recTab); comboFmt->addItems({"mp4","mkv"}); comboFmt->setCurrentText(m_settings->recording.videoFormat);
    auto *comboQual = new QComboBox(recTab); comboQual->addItems({"high","medium","low"}); comboQual->setCurrentText(m_settings->recording.recordingQuality);
    recForm->addRow("Output dir:", hbox);
    recForm->addRow("Format:", comboFmt);
    recForm->addRow("Quality:", comboQual);
    auto *btnOpen = new QPushButton("Open folder", recTab);
    connect(btnOpen, &QPushButton::clicked, this, [editOutDir](){
        QDesktopServices::openUrl(QUrl::fromLocalFile(editOutDir->text()));
    });
    recForm->addRow(btnOpen);
    tabs->addTab(recTab, "Recording");

    layout->addWidget(tabs);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);

    if (dlg.exec() == QDialog::Accepted) {
        m_settings->display.maxSize = spinSize->value();
        m_settings->display.maxFps = spinFps->value();
        m_settings->display.videoBitrate = spinBitrate->value()*1000;
        m_settings->display.videoCodec = comboCodec->currentText();
        m_settings->display.fullscreen = chkFullscreen->isChecked();
        m_settings->display.stayAwake = chkStayAwake->isChecked();
        m_settings->display.showTouches = chkShowTouches->isChecked();
        m_settings->display.disableScreensaver = chkDisableSaver->isChecked();

        m_settings->input.mouseControl = chkMouse->isChecked();
        m_settings->input.keyboardControl = chkKeyboard->isChecked();
        m_settings->input.clipboardAutosync = chkClipboard->isChecked();
        m_settings->input.turnScreenOff = chkTurnOff->isChecked();

        m_settings->connection.autoReconnect = chkAuto->isChecked();

        m_settings->recording.outputDir = editOutDir->text();
        m_settings->recording.videoFormat = comboFmt->currentText();
        m_settings->recording.recordingQuality = comboQual->currentText();

        m_settings->save();
        m_scrcpy->updateSettings(m_settings);
        m_clipboard->setAutosync(m_settings->input.clipboardAutosync);
        m_fpsLabel->setText(QString::number(m_settings->display.maxFps) + " FPS");
        statusBar()->showMessage("Settings saved", 3000);
    }
}

void MainWindow::onError(const QString &msg) {
    statusBar()->showMessage("Error: " + msg, 4000);
}

} // namespace AndroidControl
