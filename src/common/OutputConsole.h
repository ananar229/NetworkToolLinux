#pragma once

#include <QPlainTextEdit>
#include <QString>

// The white, monospaced, read-only output box used by every tool tab,
// mirroring the console area at the bottom of each Network Utility pane.
class OutputConsole : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit OutputConsole(QWidget *parent = nullptr);

    void appendPlain(const QString &text);
    void appendNotice(const QString &text);
    void appendError(const QString &text);
};
