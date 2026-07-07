#pragma once

#include <QWidget>

class QRadioButton;
class QPushButton;
class OutputConsole;

// "Netstat" tab: routing table / protocol statistics / multicast
// memberships, read directly from /proc/net/* so no `netstat`/`ip`/`ss`
// binary is required.
class NetstatTab : public QWidget {
    Q_OBJECT

public:
    explicit NetstatTab(QWidget *parent = nullptr);

private slots:
    void run();

private:
    enum class Mode { Routing, ProtocolStats, Multicast };
    Mode currentMode() const;

    QRadioButton *m_routingRadio;
    QRadioButton *m_statsRadio;
    QRadioButton *m_multicastRadio;
    QPushButton *m_runButton;
    OutputConsole *m_output;
};
