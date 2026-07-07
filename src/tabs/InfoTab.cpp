#include "InfoTab.h"

#include <QComboBox>
#include <QFile>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QNetworkInterface>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>

InfoTab::InfoTab(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *pickerRow = new QHBoxLayout();
    auto *pickerLabel = new QLabel(tr("Please select a network interface for information:"));
    m_interfaceCombo = new QComboBox();
    pickerRow->addWidget(pickerLabel);
    pickerRow->addWidget(m_interfaceCombo, 1);
    layout->addLayout(pickerRow);

    auto *columns = new QHBoxLayout();
    columns->setSpacing(16);

    auto *interfaceBox = new QGroupBox(tr("Interface Information"));
    auto *interfaceForm = new QFormLayout(interfaceBox);
    m_hardwareAddress = makeValueLabel();
    m_ipAddresses = makeValueLabel();
    m_linkSpeed = makeValueLabel();
    m_linkStatus = makeValueLabel();
    m_mtu = makeValueLabel();
    interfaceForm->addRow(tr("Hardware Address:"), m_hardwareAddress);
    interfaceForm->addRow(tr("IP Address(es):"), m_ipAddresses);
    interfaceForm->addRow(tr("Link Speed:"), m_linkSpeed);
    interfaceForm->addRow(tr("Link Status:"), m_linkStatus);
    interfaceForm->addRow(tr("MTU:"), m_mtu);

    auto *statsBox = new QGroupBox(tr("Transfer Statistics"));
    auto *statsForm = new QFormLayout(statsBox);
    m_packetsReceived = makeValueLabel();
    m_receiveErrors = makeValueLabel();
    m_packetsSent = makeValueLabel();
    m_sendErrors = makeValueLabel();
    m_collisions = makeValueLabel();
    statsForm->addRow(tr("Packets Received:"), m_packetsReceived);
    statsForm->addRow(tr("Receive Errors:"), m_receiveErrors);
    statsForm->addRow(tr("Packets Sent:"), m_packetsSent);
    statsForm->addRow(tr("Send Errors:"), m_sendErrors);
    statsForm->addRow(tr("Collisions:"), m_collisions);

    columns->addWidget(interfaceBox);
    columns->addWidget(statsBox);
    layout->addLayout(columns);
    layout->addStretch(1);

    populateInterfaces();

    connect(m_interfaceCombo, &QComboBox::currentTextChanged, this, &InfoTab::refresh);

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(2000);
    connect(m_refreshTimer, &QTimer::timeout, this, &InfoTab::refresh);
    m_refreshTimer->start();

    refresh();
}

QLabel *InfoTab::makeValueLabel() {
    auto *label = new QLabel(tr("--"));
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

void InfoTab::populateInterfaces() {
    m_interfaceCombo->clear();
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const auto &iface : interfaces) {
        m_interfaceCombo->addItem(iface.name());
    }
}

QString InfoTab::readSysfsFile(const QString &interfaceName, const QString &file) {
    QFile f(QStringLiteral("/sys/class/net/%1/%2").arg(interfaceName, file));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    return QString::fromLocal8Bit(f.readAll()).trimmed();
}

void InfoTab::refresh() {
    const QString name = m_interfaceCombo->currentText();
    if (name.isEmpty()) {
        return;
    }

    QNetworkInterface iface = QNetworkInterface::interfaceFromName(name);
    if (!iface.isValid()) {
        return;
    }

    m_hardwareAddress->setText(iface.hardwareAddress().isEmpty() ? tr("--") : iface.hardwareAddress());

    QStringList addresses;
    for (const auto &entry : iface.addressEntries()) {
        addresses << entry.ip().toString();
    }
    m_ipAddresses->setText(addresses.isEmpty() ? tr("none") : addresses.join(", "));

    const QString speed = readSysfsFile(name, "speed");
    if (speed.isEmpty() || speed == "-1") {
        m_linkSpeed->setText(tr("N/A"));
    } else {
        m_linkSpeed->setText(tr("%1 Mb/s").arg(speed));
    }

    const QString operState = readSysfsFile(name, "operstate");
    const bool isUp = iface.flags().testFlag(QNetworkInterface::IsUp) &&
                       iface.flags().testFlag(QNetworkInterface::IsRunning);
    m_linkStatus->setText(isUp ? tr("Active (%1)").arg(operState.isEmpty() ? tr("up") : operState)
                                : tr("Inactive (%1)").arg(operState.isEmpty() ? tr("down") : operState));

    m_mtu->setText(iface.maximumTransmissionUnit() > 0 ? QString::number(iface.maximumTransmissionUnit())
                                                        : tr("--"));

    auto statValue = [&](const QString &file) -> QString {
        const QString raw = readSysfsFile(name, QStringLiteral("statistics/%1").arg(file));
        return raw.isEmpty() ? tr("--") : raw;
    };

    m_packetsReceived->setText(statValue("rx_packets"));
    m_receiveErrors->setText(statValue("rx_errors"));
    m_packetsSent->setText(statValue("tx_packets"));
    m_sendErrors->setText(statValue("tx_errors"));
    m_collisions->setText(statValue("collisions"));
}
