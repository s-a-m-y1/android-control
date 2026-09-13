// Provides a QCoreApplication for QProcess/QTimer-based managers under test,
// replacing gtest_main which cannot initialize Qt internals.
#include <QCoreApplication>
#include <gtest/gtest.h>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
