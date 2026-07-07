#pragma once

#include <QWidget>

class QLineEdit;
class QComboBox;
class QPushButton;
class WhoisClient;
class OutputConsole;

// "Whois" tab: query domain/IP registration info by speaking the whois
// protocol (RFC 3912) directly over TCP, no external `whois` binary needed.
class WhoisTab : public QWidget {
    Q_OBJECT

public:
    explicit WhoisTab(QWidget *parent = nullptr);

private slots:
    void run();
    void onServerQueried(const QString &server);
    void onChunkReceived(const QString &text);
    void onFinished();
    void onErrorOccurred(const QString &message);

private:
    QLineEdit *m_addressEdit;
    QComboBox *m_serverCombo;
    QPushButton *m_runButton;
    OutputConsole *m_output;
    WhoisClient *m_client;
};
