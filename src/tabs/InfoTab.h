#pragma once

#include <QWidget>

class QComboBox;
class QLabel;
class QTimer;

// "Info" tab: interface picker plus hardware/IP/link details and live
// transfer statistics, equivalent to the first tab of Network Utility.
class InfoTab : public QWidget {
    Q_OBJECT

public:
    explicit InfoTab(QWidget *parent = nullptr);

private slots:
    void refresh();

private:
    void populateInterfaces();
    QLabel *makeValueLabel();
    static QString readSysfsFile(const QString &interfaceName, const QString &file);

    QComboBox *m_interfaceCombo;
    QTimer *m_refreshTimer;

    QLabel *m_hardwareAddress;
    QLabel *m_ipAddresses;
    QLabel *m_linkSpeed;
    QLabel *m_linkStatus;
    QLabel *m_mtu;

    QLabel *m_packetsReceived;
    QLabel *m_receiveErrors;
    QLabel *m_packetsSent;
    QLabel *m_sendErrors;
    QLabel *m_collisions;
};
