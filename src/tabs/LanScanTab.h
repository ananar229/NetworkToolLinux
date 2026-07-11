#pragma once

#include <QHash>
#include <QSet>
#include <QWidget>

class QComboBox;
class QPushButton;
class QProgressBar;
class QTableWidget;
class QProcess;
class QHostInfo;

// "LAN Scan" tab: discovers hosts on the selected interface's local subnet.
// Linux offers no unprivileged way to send raw ARP requests directly, so
// host discovery works by pinging every address in the subnet (which forces
// the kernel to ARP-resolve each destination regardless of whether it
// answers the ping itself) and then reading the resulting /proc/net/arp
// table for which addresses actually resolved to a MAC. Each discovered
// host is then enriched with reverse DNS, MAC vendor and a best-effort
// IPv6 neighbor lookup (triggered via a multicast ping to populate the
// kernel's IPv6 neighbor cache, then reading `ip -6 neigh`).
class LanScanTab : public QWidget {
    Q_OBJECT

public:
    explicit LanScanTab(QWidget *parent = nullptr);

private slots:
    void toggle();

private:
    void populateInterfaces();
    void startScan();
    void stopScan();
    void dispatchNextPing();
    void onPingFinished(const QString &ip, bool ok);
    void collectArpResults();
    void startIpv6Discovery();
    void onIpv6PingDone();
    void onIpv6NeighDone();
    void finishScan();
    void onHostLookupFinished(const QHostInfo &info);

    static constexpr int kMaxConcurrent = 32;
    static constexpr int kMaxHosts = 1024;

    QComboBox *m_interfaceCombo;
    QPushButton *m_actionButton;
    QProgressBar *m_progressBar;
    QTableWidget *m_table;

    QStringList m_pendingHosts;
    int m_totalHosts = 0;
    int m_completedHosts = 0;
    QHash<QString, bool> m_pingResults;
    QSet<QProcess *> m_activeProcesses;
    QHash<QString, int> m_rowForIp; // ipv4 -> table row
    QHash<int, QString> m_lookupIdToIp;
    QString m_scanInterfaceName;
    QString m_pendingIpv6Output;
    bool m_scanning = false;
    bool m_stopRequested = false;
};
