#pragma once

#include <QHostAddress>
#include <QSet>
#include <QWidget>

class QLineEdit;
class QSpinBox;
class QPushButton;
class QProgressBar;
class QTcpSocket;
class QHostInfo;
class OutputConsole;

// "Port Scan" tab: asynchronous TCP connect scan over a port range,
// implemented directly with QTcpSocket (no external scanner needed).
class PortScanTab : public QWidget {
    Q_OBJECT

public:
    explicit PortScanTab(QWidget *parent = nullptr);

private slots:
    void toggle();
    void onLookupFinished(const QHostInfo &info);

private:
    void startScan();
    void stopScan();
    void finishScan();
    void dispatchNext();
    void completeSocket(QTcpSocket *socket, bool open);

    static constexpr int kMaxConcurrent = 64;
    static constexpr int kTimeoutMs = 500;

    QLineEdit *m_addressEdit;
    QSpinBox *m_startPort;
    QSpinBox *m_endPort;
    QPushButton *m_actionButton;
    QProgressBar *m_progressBar;
    OutputConsole *m_output;

    QHostAddress m_targetAddress;
    QSet<QTcpSocket *> m_activeSockets;
    int m_nextPort = 0;
    int m_endPortValue = 0;
    int m_openCount = 0;
    int m_scannedCount = 0;
    bool m_scanning = false;
    bool m_stopRequested = false;
};
