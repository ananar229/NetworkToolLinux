#include "PortScanTab.h"

#include "../common/OutputConsole.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QHostInfo>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTcpSocket>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

PortScanTab::PortScanTab(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    layout->addWidget(new QLabel(tr("Enter the network address you would like to scan:")));
    m_addressEdit = new QLineEdit();
    m_addressEdit->setPlaceholderText(tr("e.g. example.com"));
    layout->addWidget(m_addressEdit);

    auto *rangeGrid = new QGridLayout();
    rangeGrid->addWidget(new QLabel(tr("Only test ports between")), 0, 0);
    m_startPort = new QSpinBox();
    m_startPort->setRange(1, 65535);
    m_startPort->setValue(1);
    rangeGrid->addWidget(m_startPort, 0, 1);
    rangeGrid->addWidget(new QLabel(tr("and")), 0, 2);
    m_endPort = new QSpinBox();
    m_endPort->setRange(1, 65535);
    m_endPort->setValue(1024);
    rangeGrid->addWidget(m_endPort, 0, 3);
    rangeGrid->setColumnStretch(4, 1);
    layout->addLayout(rangeGrid);

    auto *buttonRow = new QHBoxLayout();
    m_progressBar = new QProgressBar();
    m_progressBar->setTextVisible(false);
    buttonRow->addWidget(m_progressBar, 1);
    m_actionButton = new QPushButton(tr("Scan"));
    m_actionButton->setDefault(true);
    buttonRow->addWidget(m_actionButton);
    layout->addLayout(buttonRow);

    auto *warningLabel =
        new QLabel(tr("Note: Port scanning may be restricted or illegal in some countries. Only scan "
                       "systems you have permission to test."));
    warningLabel->setWordWrap(true);
    warningLabel->setStyleSheet(QStringLiteral("color: #b06a00;"));
    layout->addWidget(warningLabel);

    m_output = new OutputConsole();
    layout->addWidget(m_output, 1);

    connect(m_actionButton, &QPushButton::clicked, this, &PortScanTab::toggle);
}

void PortScanTab::toggle() {
    if (m_scanning) {
        stopScan();
    } else {
        startScan();
    }
}

void PortScanTab::startScan() {
    const QString address = m_addressEdit->text().trimmed();
    if (address.isEmpty()) {
        m_output->appendError(tr("Please enter a network address.\n"));
        return;
    }

    int start = m_startPort->value();
    int end = m_endPort->value();
    if (start > end) {
        std::swap(start, end);
    }

    m_output->clear();
    m_scanning = true;
    m_stopRequested = false;
    m_openCount = 0;
    m_scannedCount = 0;
    m_nextPort = start;
    m_endPortValue = end;

    m_progressBar->setRange(0, end - start + 1);
    m_progressBar->setValue(0);

    m_actionButton->setText(tr("Stop"));
    m_addressEdit->setEnabled(false);
    m_startPort->setEnabled(false);
    m_endPort->setEnabled(false);

    m_output->appendNotice(tr("Resolving %1...\n").arg(address));
    QHostInfo::lookupHost(address, this, &PortScanTab::onLookupFinished);
}

void PortScanTab::onLookupFinished(const QHostInfo &info) {
    if (!m_scanning) {
        return;
    }
    if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
        m_output->appendError(tr("Could not resolve host: %1\n").arg(info.errorString()));
        finishScan();
        return;
    }

    m_targetAddress = info.addresses().constFirst();
    m_output->appendNotice(tr("Scanning %1, ports %2-%3...\n")
                                .arg(m_targetAddress.toString())
                                .arg(m_nextPort)
                                .arg(m_endPortValue));
    dispatchNext();
}

void PortScanTab::dispatchNext() {
    while (!m_stopRequested && m_activeSockets.size() < kMaxConcurrent && m_nextPort <= m_endPortValue) {
        const int port = m_nextPort++;
        auto *socket = new QTcpSocket(this);
        socket->setProperty("port", port);
        socket->setProperty("done", false);
        m_activeSockets.insert(socket);

        connect(socket, &QTcpSocket::connected, this, [this, socket] { completeSocket(socket, true); });
        connect(socket, &QAbstractSocket::errorOccurred, this,
                [this, socket](QAbstractSocket::SocketError) { completeSocket(socket, false); });
        QTimer::singleShot(kTimeoutMs, this, [this, socket] {
            if (m_activeSockets.contains(socket) && socket->state() == QAbstractSocket::ConnectingState) {
                socket->abort();
                completeSocket(socket, false);
            }
        });

        socket->connectToHost(m_targetAddress, static_cast<quint16>(port));
    }

    if (m_activeSockets.isEmpty() && m_nextPort > m_endPortValue) {
        finishScan();
    }
}

void PortScanTab::completeSocket(QTcpSocket *socket, bool open) {
    if (!m_activeSockets.contains(socket) || socket->property("done").toBool()) {
        return;
    }
    socket->setProperty("done", true);

    const int port = socket->property("port").toInt();
    m_scannedCount++;
    m_progressBar->setValue(m_scannedCount);
    if (open) {
        m_openCount++;
        m_output->appendPlain(tr("Open TCP Port: %1\n").arg(port));
    }

    m_activeSockets.remove(socket);
    socket->disconnect(this);
    socket->deleteLater();

    if (!m_stopRequested) {
        dispatchNext();
    } else if (m_activeSockets.isEmpty()) {
        finishScan();
    }
}

void PortScanTab::stopScan() {
    m_stopRequested = true;
    const auto sockets = m_activeSockets;
    for (auto *socket : sockets) {
        socket->disconnect(this);
        socket->abort();
        socket->deleteLater();
    }
    m_activeSockets.clear();
    finishScan();
}

void PortScanTab::finishScan() {
    if (!m_scanning) {
        return;
    }
    m_scanning = false;
    m_actionButton->setText(tr("Scan"));
    m_addressEdit->setEnabled(true);
    m_startPort->setEnabled(true);
    m_endPort->setEnabled(true);
    m_output->appendNotice(
        tr("\nScan finished: %1 open port(s) found out of %2 scanned.\n").arg(m_openCount).arg(m_scannedCount));
}
