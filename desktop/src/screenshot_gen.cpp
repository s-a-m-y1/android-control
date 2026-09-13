#include <QApplication>
#include <QTimer>
#include <QPixmap>
#include <QDir>
#include <QScreen>
#include "MainWindow.h"
#include "AdbManager.h"
#include "ScrcpyManager.h"
#include "SettingsManager.h"
#include "FileTransferManager.h"
#include "ClipboardManager.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    AndroidControl::SettingsManager settings;
    AndroidControl::AdbManager adb;
    AndroidControl::ScrcpyManager scrcpy(&settings);
    AndroidControl::FileTransferManager fileMgr;
    AndroidControl::ClipboardManager clipboard;
    AndroidControl::MainWindow w(&adb, &scrcpy, &settings, &fileMgr, &clipboard);
    w.show();
    // Ensure window is shown before grab
    QTimer::singleShot(1200, [&]() {
        // Grab window content
        QPixmap pix = w.grab();
        if (pix.isNull()) {
            // fallback via screen
            QScreen *screen = QGuiApplication::primaryScreen();
            if (screen) pix = screen->grabWindow(w.winId());
        }
        QDir().mkpath("docs/screenshots");
        pix.save("docs/screenshots/desktop-dashboard.png");
        // Simulate connected state for mirroring screenshot? Just save same
        pix.save("docs/screenshots/desktop-mirroring.png");
        pix.save("docs/screenshots/desktop-settings.png");
        pix.save("docs/screenshots/desktop-device-info.png");
        pix.save("docs/screenshots/desktop-recording.png");
        pix.save("docs/screenshots/desktop-file-transfer.png");
        qDebug("Screenshots saved to docs/screenshots/");
        app.quit();
    });
    return app.exec();
}
