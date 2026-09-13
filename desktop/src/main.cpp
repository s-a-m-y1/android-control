#include <QApplication>
#include <QSurfaceFormat>
#include "AdbManager.h"
#include "ScrcpyManager.h"
#include "SettingsManager.h"
#include "FileTransferManager.h"
#include "ClipboardManager.h"
#include "MainWindow.h"
#include <QDebug>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <cstdio>

int main(int argc, char *argv[]) {
    // Enable high DPI scaling
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("Android Control");
    app.setOrganizationName("AndroidControl");
    app.setApplicationVersion("1.0.0");
    app.setDesktopFileName("com.github.androidcontrol");

    // Modern dark theme
    {
        QFile th(":themes/dark-theme.qss");
        if (th.open(QIODevice::ReadOnly | QIODevice::Text))
            app.setStyleSheet(QString::fromUtf8(th.readAll()));
    }

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

    // --mirror [serial] parsing (also forwarded to an already-running instance)
    const int mirrorIdx = cli.indexOf("--mirror");
    auto mirrorSerial = [&]() -> QString {
        if (mirrorIdx >= 0 && mirrorIdx + 1 < cli.size() && !cli.at(mirrorIdx + 1).startsWith('-'))
            return cli.at(mirrorIdx + 1);
        return {};
    }();

    // Single instance: if another android-control is already running, tell it
    // to show its window instead of launching a second copy.
    {
        QLocalSocket probe;
        probe.connectToServer("android-control-singleton");
        if (probe.waitForConnected(300)) {
            probe.write(mirrorIdx >= 0 ? ("MIRROR " + mirrorSerial).toUtf8() : QByteArrayLiteral("SHOW"));
            probe.waitForBytesWritten(300);
            qInfo() << "Already running — showing the existing window";
            return 0;
        }
    }

    AndroidControl::SettingsManager settings;
    AndroidControl::AdbManager adb;
    AndroidControl::ScrcpyManager scrcpy(&settings);
    AndroidControl::FileTransferManager fileMgr;
    AndroidControl::ClipboardManager clipboard;
    clipboard.setAutosync(settings.input.clipboardAutosync);

    AndroidControl::MainWindow w(&adb, &scrcpy, &settings, &fileMgr, &clipboard);
    w.show();

    // Listen for second launches: SHOW reveals the window, MIRROR starts mirroring.
    QLocalServer instanceServer;
    QLocalServer::removeServer("android-control-singleton");
    instanceServer.listen("android-control-singleton");
    QObject::connect(&instanceServer, &QLocalServer::newConnection, [&]() {
        auto *conn = instanceServer.nextPendingConnection();
        auto handleCommand = [conn, &w, &scrcpy]() {
            const QString cmd = QString::fromUtf8(conn->readAll()).trimmed();
            if (cmd.isEmpty()) return;
            qInfo() << "Instance command received:" << cmd;
            w.show();
            w.raise();
            w.activateWindow();
            if (cmd.startsWith("MIRROR") && !scrcpy.isAnyMirroring())
                w.startMirroringFor(cmd.mid(6).trimmed());
        };
        QObject::connect(conn, &QLocalSocket::readyRead, conn, handleCommand);
        // The second instance often writes and exits immediately, so the data can
        // land before readyRead is connected — handle it on disconnect too.
        QObject::connect(conn, &QLocalSocket::disconnected, conn, [conn, handleCommand]() mutable {
            handleCommand();
            conn->deleteLater();
        });
        if (conn->bytesAvailable() > 0) handleCommand();
    });

    if (mirrorIdx >= 0)
        w.startMirroringFor(mirrorSerial);

    int ret = app.exec();
    scrcpy.stopAll();
    qInfo() << "Android Control exited with" << ret;
    return ret;
}
