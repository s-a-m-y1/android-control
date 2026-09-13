#include <QApplication>
#include <QSurfaceFormat>
#include "AdbManager.h"
#include "ScrcpyManager.h"
#include "SettingsManager.h"
#include "FileTransferManager.h"
#include "ClipboardManager.h"
#include "MainWindow.h"
#include <spdlog/spdlog.h>

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

    spdlog::set_level(spdlog::level::info);
    spdlog::info("Android Control starting...");

    AndroidControl::SettingsManager settings;
    AndroidControl::AdbManager adb;
    AndroidControl::ScrcpyManager scrcpy(&settings);
    AndroidControl::FileTransferManager fileMgr;
    AndroidControl::ClipboardManager clipboard;
    clipboard.setAutosync(settings.input.clipboardAutosync);

    AndroidControl::MainWindow w(&adb, &scrcpy, &settings, &fileMgr, &clipboard);
    w.show();

    int ret = app.exec();
    scrcpy.stopAll();
    spdlog::info("Android Control exited with {}", ret);
    return ret;
}
