#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QTcpSocket;
class OutputConsole;

// "Finger" tab: query user information from a remote host by speaking the
// finger protocol (RFC 1288) directly over TCP port 79, no external
// `finger` binary needed.
class FingerTab : public QWidget {
    Q_OBJECT

public:
    explicit FingerTab(QWidget *parent = nullptr);

private slots:
    void run();
    void onConnected();
    void onReadyRead();
    void onFinished();
    void onErrorOccurred();

private:
    QLineEdit *m_addressEdit;
    QPushButton *m_runButton;
    OutputConsole *m_output;
    QTcpSocket *m_socket;
    QString m_user;
};
