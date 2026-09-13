#pragma once
#include "AdbManager.h"
#include "ScrcpyManager.h"
#include "SettingsManager.h"
#include "FileTransferManager.h"
#include "ClipboardManager.h"
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QTimer>

namespace AndroidControl {

class DeviceWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(AdbManager *adb, ScrcpyManager *scrcpy, SettingsManager *settings,
                        FileTransferManager *fileMgr, ClipboardManager *clipboard,
                        QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onDevicesUpdated(const std::vector<DeviceInfo> &devices);
    void onError(const QString &msg);
    void onMirrorRequested(const QString &serial);
    void onDeviceSelected(const QString &serial);
    void onScreenshot();
    void onRecord();
    void onRotate();
    void onFullscreen();
    void onDisconnect();
    void onPushFile();
    void onPullFile();
    void onCopyClipboard();
    void onPasteClipboard();
    void onSettings();
    void refreshDevices();
    void updatePreview();
    void updateStatusBar();
    void onMirroringStarted(const QString &serial);
    void onMirroringFailed(const QString &serial, const QString &error);

private:
    void setupUi();
    void setupMenu();
    void checkDependencies();
    void showAdbGuide();
    void showUnauthorizedDialog(const DeviceInfo &info);
    void rebuildDeviceList();
    DeviceInfo* findSelectedDevice();

    AdbManager *m_adb = nullptr;
    ScrcpyManager *m_scrcpy = nullptr;
    SettingsManager *m_settings = nullptr;
    FileTransferManager *m_fileMgr = nullptr;
    ClipboardManager *m_clipboard = nullptr;

    std::vector<DeviceInfo> m_devices;
    QString m_selectedSerial;

    // UI
    QWidget *m_central = nullptr;
    QVBoxLayout *m_mainLayout = nullptr;
    QVBoxLayout *m_deviceListLayout = nullptr;
    QWidget *m_deviceListContainer = nullptr;
    QLabel *m_deviceCountLabel = nullptr;
    QLabel *m_bannerLabel = nullptr;
    QWidget *m_banner = nullptr;

    // Preview
    QLabel *m_previewIcon = nullptr;
    QLabel *m_previewTitle = nullptr;
    QLabel *m_previewSubtitle = nullptr;
    QLabel *m_previewHint = nullptr;
    QPushButton *m_centerMirrorButton = nullptr;
    QFrame *m_previewFrame = nullptr;

    // Device info
    QWidget *m_infoGroup = nullptr;
    QLabel *m_infoLabel = nullptr;

    // Status bar
    QLabel *m_statusDot = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_fpsLabel = nullptr;
    QLabel *m_usbLabel = nullptr;

    // Action buttons
    QPushButton *m_btnScreenshot = nullptr;
    QPushButton *m_btnRecord = nullptr;
    QPushButton *m_btnRotate = nullptr;
    QPushButton *m_btnFullscreen = nullptr;
    QPushButton *m_btnDisconnect = nullptr;
    QPushButton *m_btnBack = nullptr;
    QPushButton *m_btnHome = nullptr;
    QPushButton *m_btnRecent = nullptr;
    QPushButton *m_btnPush = nullptr;
    QPushButton *m_btnPull = nullptr;

    QTimer *m_refreshTimer = nullptr;
    bool m_isRecording = false;
};

} // namespace AndroidControl
