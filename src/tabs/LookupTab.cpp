#include "LookupTab.h"

#include "../common/CommandRunner.h"
#include "../common/OutputConsole.h"

#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LookupTab::LookupTab(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    layout->addWidget(new QLabel(tr("Enter an internet address to lookup:")));
    m_addressEdit = new QLineEdit();
    m_addressEdit->setPlaceholderText(tr("e.g. example.com"));
    layout->addWidget(m_addressEdit);

    auto *typeRow = new QHBoxLayout();
    typeRow->addWidget(new QLabel(tr("Select the type of lookup you would like to perform:")));
    m_typeCombo = new QComboBox();
    m_typeCombo->addItem(tr("Default (A/AAAA)"), QString());
    m_typeCombo->addItem(tr("IPv4 Address (A)"), QStringLiteral("A"));
    m_typeCombo->addItem(tr("IPv6 Address (AAAA)"), QStringLiteral("AAAA"));
    m_typeCombo->addItem(tr("Mail Exchanger (MX)"), QStringLiteral("MX"));
    m_typeCombo->addItem(tr("Name Server (NS)"), QStringLiteral("NS"));
    m_typeCombo->addItem(tr("Text (TXT)"), QStringLiteral("TXT"));
    m_typeCombo->addItem(tr("Canonical Name (CNAME)"), QStringLiteral("CNAME"));
    m_typeCombo->addItem(tr("Start of Authority (SOA)"), QStringLiteral("SOA"));
    m_typeCombo->addItem(tr("Any Available Records (ANY)"), QStringLiteral("ANY"));
    typeRow->addWidget(m_typeCombo, 1);
    layout->addLayout(typeRow);

    auto *buttonRow = new QHBoxLayout();
    m_runButton = new QPushButton(tr("Lookup"));
    m_runButton->setDefault(true);
    buttonRow->addStretch(1);
    buttonRow->addWidget(m_runButton);
    layout->addLayout(buttonRow);

    m_output = new OutputConsole();
    layout->addWidget(m_output, 1);

    m_runner = new CommandRunner(this);
    connect(m_runner, &CommandRunner::outputReceived, m_output, &OutputConsole::appendPlain);
    connect(m_runner, &CommandRunner::finished, this, &LookupTab::onFinished);
    connect(m_runner, &CommandRunner::failedToStart, this, &LookupTab::onFailedToStart);
    connect(m_runButton, &QPushButton::clicked, this, &LookupTab::run);
    connect(m_addressEdit, &QLineEdit::returnPressed, this, &LookupTab::run);
}

void LookupTab::run() {
    const QString address = m_addressEdit->text().trimmed();
    if (address.isEmpty()) {
        m_output->appendError(tr("Please enter an internet address.\n"));
        return;
    }

    m_output->clear();
    m_runButton->setEnabled(false);

    QStringList args;
    const QString recordType = m_typeCombo->currentData().toString();
    if (!recordType.isEmpty()) {
        args << "-t" << recordType;
    }
    args << address;
    m_runner->start("host", args);
}

void LookupTab::onFinished(int) {
    m_runButton->setEnabled(true);
}

void LookupTab::onFailedToStart(const QString &program) {
    m_output->appendError(tr("Failed to start '%1'. Is it installed?\n").arg(program));
    onFinished(-1);
}
