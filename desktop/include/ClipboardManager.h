#pragma once
#include <QObject>
#include <QString>
#include <QClipboard>

namespace AndroidControl {

class ClipboardManager : public QObject {
    Q_OBJECT
public:
    explicit ClipboardManager(QObject *parent = nullptr);

    bool copyToDevice(const QString &serial, const QString &text);
    QString pasteFromDevice(const QString &serial);
    bool syncClipboard(const QString &serial);

    void setAutosync(bool enabled) { m_autosync = enabled; }
    bool autosync() const { return m_autosync; }

signals:
    void clipboardChanged(const QString &text);
    void syncFailed(const QString &error);

private slots:
    void onClipboardChanged();

private:
    QClipboard *m_clipboard = nullptr;
    bool m_autosync = true;
    QString m_lastText;
};

} // namespace AndroidControl
