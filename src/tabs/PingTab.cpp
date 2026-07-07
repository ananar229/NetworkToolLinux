#include "PingTab.h"

#include "../common/CommandRunner.h"
#include "../common/OutputConsole.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

PingTab::PingTab(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *grid = new QGridLayout();
    grid->addWidget(new QLabel(tr("Enter a network address to ping:")), 0, 0, 1, 3);
    m_addressEdit = new QLineEdit();
    m_addressEdit->setPlaceholderText(tr("e.g. 8.8.8.8 or example.com"));
    grid->addWidget(m_addressEdit, 1, 0, 1, 3);

    m_limitCheckBox = new QCheckBox(tr("Send only"));
    m_countSpinBox = new QSpinBox();
    m_countSpinBox->setRange(1, 10000);
    m_countSpinBox->setValue(10);
    m_countSpinBox->setEnabled(false);
    auto *pingsLabel = new QLabel(tr("pings"));
    grid->addWidget(m_limitCheckBox, 2, 0);
    grid->addWidget(m_countSpinBox, 2, 1);
    grid->addWidget(pingsLabel, 2, 2);
    grid->setColumnStretch(2, 1);
    layout->addLayout(grid);

    connect(m_limitCheckBox, &QCheckBox::toggled, m_countSpinBox, &QSpinBox::setEnabled);

    auto *buttonRow = new QHBoxLayout();
    m_actionButton = new QPushButton(tr("Ping"));
    m_actionButton->setDefault(true);
    buttonRow->addStretch(1);
    buttonRow->addWidget(m_actionButton);
    layout->addLayout(buttonRow);

    m_output = new OutputConsole();
    layout->addWidget(m_output, 1);

    m_runner = new CommandRunner(this);
    connect(m_runner, &CommandRunner::outputReceived, m_output, &OutputConsole::appendPlain);
    connect(m_runner, &CommandRunner::started, this, &PingTab::onStarted);
    connect(m_runner, &CommandRunner::finished, this, &PingTab::onFinished);
    connect(m_runner, &CommandRunner::failedToStart, this, &PingTab::onFailedToStart);
    connect(m_actionButton, &QPushButton::clicked, this, &PingTab::toggle);
}

void PingTab::toggle() {
    if (m_runner->isRunning()) {
        m_runner->stop();
        return;
    }

    const QString address = m_addressEdit->text().trimmed();
    if (address.isEmpty()) {
        m_output->appendError(tr("Please enter a network address.\n"));
        return;
    }

    m_output->clear();
    QStringList args;
    if (m_limitCheckBox->isChecked()) {
        args << "-c" << QString::number(m_countSpinBox->value());
    }
    args << address;
    m_runner->start("ping", args);
}

void PingTab::onStarted() {
    m_actionButton->setText(tr("Stop"));
    m_addressEdit->setEnabled(false);
}

void PingTab::onFinished(int) {
    m_actionButton->setText(tr("Ping"));
    m_addressEdit->setEnabled(true);
}

void PingTab::onFailedToStart(const QString &program) {
    m_output->appendError(tr("Failed to start '%1'. Is it installed?\n").arg(program));
    onFinished(-1);
}
