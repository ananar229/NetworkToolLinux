#include "WhoisTab.h"

#include "../common/OutputConsole.h"
#include "../common/WhoisClient.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

WhoisTab::WhoisTab(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    layout->addWidget(new QLabel(tr("Enter the network address you would like to whois:")));
    m_addressEdit = new QLineEdit();
    m_addressEdit->setPlaceholderText(tr("e.g. example.com"));
    layout->addWidget(m_addressEdit);

    auto *serverRow = new QHBoxLayout();
    serverRow->addWidget(new QLabel(tr("Select a whois server:")));
    m_serverCombo = new QComboBox();
    m_serverCombo->addItem(tr("Automatic (recommended)"), QString());
    m_serverCombo->addItem(tr("whois.internic.net"), QStringLiteral("whois.internic.net"));
    m_serverCombo->addItem(tr("whois.arin.net"), QStringLiteral("whois.arin.net"));
    m_serverCombo->addItem(tr("whois.iana.org"), QStringLiteral("whois.iana.org"));
    m_serverCombo->addItem(tr("whois.ripe.net"), QStringLiteral("whois.ripe.net"));
    serverRow->addWidget(m_serverCombo, 1);
    layout->addLayout(serverRow);

    auto *buttonRow = new QHBoxLayout();
    m_runButton = new QPushButton(tr("Whois"));
    m_runButton->setDefault(true);
    buttonRow->addStretch(1);
    buttonRow->addWidget(m_runButton);
    layout->addLayout(buttonRow);

    m_output = new OutputConsole();
    layout->addWidget(m_output, 1);

    m_client = new WhoisClient(this);
    connect(m_client, &WhoisClient::serverQueried, this, &WhoisTab::onServerQueried);
    connect(m_client, &WhoisClient::chunkReceived, this, &WhoisTab::onChunkReceived);
    connect(m_client, &WhoisClient::finished, this, &WhoisTab::onFinished);
    connect(m_client, &WhoisClient::errorOccurred, this, &WhoisTab::onErrorOccurred);
    connect(m_runButton, &QPushButton::clicked, this, &WhoisTab::run);
    connect(m_addressEdit, &QLineEdit::returnPressed, this, &WhoisTab::run);
}

void WhoisTab::run() {
    const QString address = m_addressEdit->text().trimmed();
    if (address.isEmpty()) {
        m_output->appendError(tr("Please enter a network address.\n"));
        return;
    }

    m_output->clear();
    m_runButton->setEnabled(false);

    const QString server = m_serverCombo->currentData().toString();
    m_client->query(address, server);
}

void WhoisTab::onServerQueried(const QString &server) {
    m_output->appendNotice(tr("Querying %1 ...\n").arg(server));
}

void WhoisTab::onChunkReceived(const QString &text) {
    m_output->appendPlain(text);
}

void WhoisTab::onFinished() {
    m_runButton->setEnabled(true);
}

void WhoisTab::onErrorOccurred(const QString &message) {
    m_output->appendError(tr("Whois query failed: %1\n").arg(message));
}
