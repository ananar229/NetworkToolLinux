#include "LanScanTab.h"

#include "../common/OuiVendors.h"

#include <QColor>
#include <QComboBox>
#include <QFile>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHostInfo>
#include <QLabel>
#include <QNetworkInterface>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <memory>

namespace {

enum Column {
    ColIPv4 = 0,
    ColHostname,
    ColPing,
    ColIPv6Local,
    ColIPv6Global,
    ColMac,
    ColVendor,
    ColCount,
};

QString normalizeMac(const QString &mac) { return mac.trimmed().toUpper(); }

} // namespace

LanScanTab::LanScanTab(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *pickerRow = new QHBoxLayout();
    pickerRow->addWidget(new QLabel(tr("Network interface to scan:")));
    m_interfaceCombo = new QComboBox();
    pickerRow->addWidget(m_interfaceCombo, 1);
    layout->addLayout(pickerRow);

    auto *buttonRow = new QHBoxLayout();
    m_progressBar = new QProgressBar();
    m_progressBar->setTextVisible(false);
    buttonRow->addWidget(m_progressBar, 1);
    m_actionButton = new QPushButton(tr("Scan"));
    m_actionButton->setDefault(true);
    buttonRow->addWidget(m_actionButton);
    layout->addLayout(buttonRow);

    m_table = new QTableWidget(0, ColCount);
    m_table->setHorizontalHeaderLabels({tr("IPv4 Address"), tr("Hostname"), tr("Ping"), tr("IPv6 Address (Local)"),
                                         tr("IPv6 Address (Global)"), tr("MAC Address"), tr("Vendor")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    layout->addWidget(m_table, 1);

    populateInterfaces();

    connect(m_actionButton, &QPushButton::clicked, this, &LanScanTab::toggle);
}

void LanScanTab::populateInterfaces() {
    m_interfaceCombo->clear();
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const auto &iface : interfaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            continue;
        }
        bool hasIPv4 = false;
        for (const auto &entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                hasIPv4 = true;
                break;
            }
        }
        if (hasIPv4) {
            m_interfaceCombo->addItem(iface.name());
        }
    }
}

void LanScanTab::toggle() {
    if (m_scanning) {
        stopScan();
    } else {
        startScan();
    }
}

void LanScanTab::startScan() {
    const QString name = m_interfaceCombo->currentText();
    if (name.isEmpty()) {
        return;
    }

    QNetworkInterface iface = QNetworkInterface::interfaceFromName(name);
    QHostAddress localIp, netmask;
    for (const auto &entry : iface.addressEntries()) {
        if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
            localIp = entry.ip();
            netmask = entry.netmask();
            break;
        }
    }
    if (localIp.isNull() || netmask.isNull()) {
        return;
    }

    const quint32 ipInt = localIp.toIPv4Address();
    const quint32 maskInt = netmask.toIPv4Address();
    const quint32 network = ipInt & maskInt;
    const quint32 broadcast = network | ~maskInt;
    quint32 hostCount = (broadcast > network + 1) ? (broadcast - network - 1) : 0;
    if (hostCount > kMaxHosts) {
        hostCount = kMaxHosts;
    }

    m_pendingHosts.clear();
    for (quint32 offset = 1; offset <= hostCount; ++offset) {
        m_pendingHosts << QHostAddress(network + offset).toString();
    }

    m_scanInterfaceName = name;
    m_totalHosts = m_pendingHosts.size();
    m_completedHosts = 0;
    m_pingResults.clear();
    m_rowForIp.clear();
    m_lookupIdToIp.clear();
    m_table->setRowCount(0);

    m_scanning = true;
    m_stopRequested = false;
    m_actionButton->setText(tr("Stop"));
    m_interfaceCombo->setEnabled(false);
    m_progressBar->setRange(0, m_totalHosts > 0 ? m_totalHosts : 1);
    m_progressBar->setValue(0);

    dispatchNextPing();
}

void LanScanTab::dispatchNextPing() {
    while (!m_stopRequested && m_activeProcesses.size() < kMaxConcurrent && !m_pendingHosts.isEmpty()) {
        const QString ip = m_pendingHosts.takeFirst();
        auto *process = new QProcess(this);
        auto reported = std::make_shared<bool>(false);
        auto reportOnce = [this, ip, reported](bool ok) {
            if (*reported) {
                return;
            }
            *reported = true;
            onPingFinished(ip, ok);
        };
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
                [reportOnce](int exitCode, QProcess::ExitStatus) { reportOnce(exitCode == 0); });
        connect(process, &QProcess::errorOccurred, this,
                [reportOnce](QProcess::ProcessError) { reportOnce(false); });
        m_activeProcesses.insert(process);
        process->start(QStringLiteral("ping"), {QStringLiteral("-c"), QStringLiteral("1"), QStringLiteral("-W"),
                                                  QStringLiteral("1"), ip});
    }

    if (m_activeProcesses.isEmpty() && m_pendingHosts.isEmpty() && m_completedHosts >= m_totalHosts) {
        collectArpResults();
    }
}

void LanScanTab::onPingFinished(const QString &ip, bool ok) {
    auto *process = qobject_cast<QProcess *>(sender());
    m_pingResults.insert(ip, ok);
    m_completedHosts++;
    m_progressBar->setValue(m_completedHosts);

    if (process) {
        m_activeProcesses.remove(process);
        process->deleteLater();
    }

    if (m_stopRequested) {
        if (m_activeProcesses.isEmpty()) {
            finishScan();
        }
        return;
    }
    dispatchNextPing();
}

void LanScanTab::collectArpResults() {
    QFile arp(QStringLiteral("/proc/net/arp"));
    if (!arp.open(QIODevice::ReadOnly | QIODevice::Text)) {
        finishScan();
        return;
    }

    const QStringList lines = QString::fromLatin1(arp.readAll()).split('\n', Qt::SkipEmptyParts);
    for (int i = 1; i < lines.size(); ++i) { // skip header line
        const QStringList fields = lines.at(i).split(' ', Qt::SkipEmptyParts);
        if (fields.size() < 6) {
            continue;
        }
        const QString ip = fields.at(0);
        const QString flags = fields.at(2);
        const QString mac = normalizeMac(fields.at(3));
        if (!m_pingResults.contains(ip)) {
            continue;
        }
        if (flags == QStringLiteral("0x0") || mac == QStringLiteral("00:00:00:00:00:00")) {
            continue; // incomplete ARP entry: nothing actually answered at L2
        }

        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_rowForIp.insert(ip, row);

        m_table->setItem(row, ColIPv4, new QTableWidgetItem(ip));
        m_table->setItem(row, ColHostname, new QTableWidgetItem(tr("--")));

        const bool pingOk = m_pingResults.value(ip, false);
        auto *pingItem = new QTableWidgetItem(pingOk ? tr("Reachable") : tr("No Reply"));
        pingItem->setForeground(pingOk ? QColor(0, 140, 0) : QColor(190, 0, 0));
        m_table->setItem(row, ColPing, pingItem);

        m_table->setItem(row, ColIPv6Local, new QTableWidgetItem(tr("--")));
        m_table->setItem(row, ColIPv6Global, new QTableWidgetItem(tr("--")));
        m_table->setItem(row, ColMac, new QTableWidgetItem(mac));

        const QString vendor = OuiVendors::lookup(mac);
        m_table->setItem(row, ColVendor, new QTableWidgetItem(vendor.isEmpty() ? tr("Unknown") : vendor));

        const int lookupId = QHostInfo::lookupHost(ip, this, &LanScanTab::onHostLookupFinished);
        m_lookupIdToIp.insert(lookupId, ip);
    }

    startIpv6Discovery();
}

void LanScanTab::onHostLookupFinished(const QHostInfo &info) {
    const QString ip = m_lookupIdToIp.take(info.lookupId());
    if (ip.isEmpty() || !m_rowForIp.contains(ip)) {
        return;
    }
    const int row = m_rowForIp.value(ip);
    const QString hostname =
        (info.error() == QHostInfo::NoError && !info.hostName().isEmpty()) ? info.hostName() : tr("--");
    if (auto *item = m_table->item(row, ColHostname)) {
        item->setText(hostname);
    }
}

void LanScanTab::startIpv6Discovery() {
    if (m_stopRequested || m_scanInterfaceName.isEmpty()) {
        finishScan();
        return;
    }

    // Populate the kernel's IPv6 neighbor cache by pinging the all-nodes
    // multicast address on this link, then read back what answered.
    auto *pingProcess = new QProcess(this);
    connect(pingProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, pingProcess](int, QProcess::ExitStatus) {
                pingProcess->deleteLater();
                onIpv6PingDone();
            });
    connect(pingProcess, &QProcess::errorOccurred, this, [this, pingProcess](QProcess::ProcessError) {
        pingProcess->deleteLater();
        onIpv6PingDone();
    });
    pingProcess->start(QStringLiteral("ping"),
                        {QStringLiteral("-6"), QStringLiteral("-c"), QStringLiteral("1"), QStringLiteral("-W"),
                         QStringLiteral("1"), QStringLiteral("-I"), m_scanInterfaceName, QStringLiteral("ff02::1")});
}

void LanScanTab::onIpv6PingDone() {
    if (m_stopRequested) {
        finishScan();
        return;
    }

    auto *neighProcess = new QProcess(this);
    connect(neighProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, neighProcess](int, QProcess::ExitStatus) {
                neighProcess->deleteLater();
                onIpv6NeighDone();
            });
    connect(neighProcess, &QProcess::errorOccurred, this, [this, neighProcess](QProcess::ProcessError) {
        neighProcess->deleteLater();
        finishScan();
    });
    m_pendingIpv6Output.clear();
    neighProcess->start(QStringLiteral("ip"),
                        {QStringLiteral("-6"), QStringLiteral("neigh"), QStringLiteral("show"),
                         QStringLiteral("dev"), m_scanInterfaceName});
    connect(neighProcess, &QProcess::readyReadStandardOutput, this, [this, neighProcess] {
        m_pendingIpv6Output += QString::fromLatin1(neighProcess->readAllStandardOutput());
    });
}

void LanScanTab::onIpv6NeighDone() {
    // Lines look like: "fe80::1234:5678:9abc:def0 dev eth0 lladdr aa:bb:cc:dd:ee:ff REACHABLE"
    QHash<QString, QStringList> localByMac;
    QHash<QString, QStringList> globalByMac;
    const QStringList lines = m_pendingIpv6Output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QStringList fields = line.split(' ', Qt::SkipEmptyParts);
        const int lladdrIndex = fields.indexOf(QStringLiteral("lladdr"));
        if (fields.isEmpty() || lladdrIndex < 0 || lladdrIndex + 1 >= fields.size()) {
            continue;
        }
        const QString address = fields.at(0);
        const QString mac = normalizeMac(fields.at(lladdrIndex + 1));
        if (address.startsWith(QStringLiteral("fe80"), Qt::CaseInsensitive)) {
            localByMac[mac] << address;
        } else {
            globalByMac[mac] << address;
        }
    }

    for (auto it = m_rowForIp.constBegin(); it != m_rowForIp.constEnd(); ++it) {
        const int row = it.value();
        auto *macItem = m_table->item(row, ColMac);
        if (!macItem) {
            continue;
        }
        const QString mac = macItem->text();
        if (localByMac.contains(mac)) {
            m_table->item(row, ColIPv6Local)->setText(localByMac.value(mac).join(QStringLiteral(", ")));
        }
        if (globalByMac.contains(mac)) {
            m_table->item(row, ColIPv6Global)->setText(globalByMac.value(mac).join(QStringLiteral(", ")));
        }
    }

    finishScan();
}

void LanScanTab::stopScan() {
    m_stopRequested = true;
    m_pendingHosts.clear();
    const auto processes = m_activeProcesses;
    for (auto *process : processes) {
        process->kill();
    }
    if (m_activeProcesses.isEmpty()) {
        finishScan();
    }
}

void LanScanTab::finishScan() {
    m_scanning = false;
    m_actionButton->setText(tr("Scan"));
    m_interfaceCombo->setEnabled(true);
    m_progressBar->setValue(m_progressBar->maximum());
}
