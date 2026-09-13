#include <QApplication>
#include <QSurfaceFormat>
#include "AdbManager.h"
#include "ScrcpyManager.h"
#include "SettingsManager.h"
#include "FileTransferManager.h"
#include "ClipboardManager.h"
#include "MainWindow.h"
#include <QDebug>
#include <cstdio>

int main(int argc, char *argv[]) {
    // Enable high DPI scaling
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("Android Control");
    app.setOrganizationName("AndroidControl");
    app.setApplicationVersion("1.0.0");
    app.setDesktopFileName("com.github.androidcontrol");

    // Optional: set OpenGL format for hardware acceleration
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(fmt);

    qInfo() << "Android Control starting...";

    // --help / --version / --mirror [serial] support
    const QStringList cli = QCoreApplication::arguments();
    if (cli.contains("--help") || cli.contains("-h")) {
        printf("Android Control — mirror & control Android via USB (ADB + scrcpy)\n"
               "Usage: android-control [options]\n"
               "  --mirror [serial]  start mirroring on launch (default: first connected device)\n"
               "  --version          print version\n"
               "  --help             show this help\n");
        return 0;
    }
    if (cli.contains("--version") || cli.contains("-v")) {
        printf("Android Control %s\n", ANDROID_CONTROL_VERSION);
        return 0;
    }

    AndroidControl::SettingsManager settings;
    AndroidControl::AdbManager adb;
    AndroidControl::ScrcpyManager scrcpy(&settings);
    AndroidControl::FileTransferManager fileMgr;
    AndroidControl::ClipboardManager clipboard;
    clipboard.setAutosync(settings.input.clipboardAutosync);

    AndroidControl::MainWindow w(&adb, &scrcpy, &settings, &fileMgr, &clipboard);
    w.show();

    // --mirror [serial]: start mirroring immediately (serial optional = first connected device)
    const int mirrorIdx = cli.indexOf("--mirror");
    if (mirrorIdx >= 0) {
        QString serial;
        if (mirrorIdx + 1 < cli.size() && !cli.at(mirrorIdx + 1).startsWith('-'))
            serial = cli.at(mirrorIdx + 1);
        w.startMirroringFor(serial);
    }

    int ret = app.exec();
    scrcpy.stopAll();
    qInfo() << "Android Control exited with" << ret;
    return ret;
}
