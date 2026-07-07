#include "TracerouteTab.h"

#include "../common/NativeTraceroute.h"
#include "../common/OutputConsole.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

TracerouteTab::TracerouteTab(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    layout->addWidget(new QLabel(tr("Enter a network address to traceroute:")));
    m_addressEdit = new QLineEdit();
    m_addressEdit->setPlaceholderText(tr("e.g. example.com"));
    layout->addWidget(m_addressEdit);

    auto *buttonRow = new QHBoxLayout();
    m_actionButton = new QPushButton(tr("Trace"));
    m_actionButton->setDefault(true);
    buttonRow->addStretch(1);
    buttonRow->addWidget(m_actionButton);
    layout->addLayout(buttonRow);

    m_output = new OutputConsole();
    layout->addWidget(m_output, 1);

    m_tracer = new NativeTraceroute(this);
    connect(m_tracer, &NativeTraceroute::hopResult, this, &TracerouteTab::onHopResult);
    connect(m_tracer, &NativeTraceroute::reachedDestination, this, &TracerouteTab::onReachedDestination);
    connect(m_tracer, &NativeTraceroute::finished, this, &TracerouteTab::onFinished);
    connect(m_tracer, &NativeTraceroute::errorOccurred, this, &TracerouteTab::onErrorOccurred);
    connect(m_actionButton, &QPushButton::clicked, this, &TracerouteTab::toggle);
}

void TracerouteTab::toggle() {
    if (m_tracer->isRunning()) {
        m_tracer->stop();
        return;
    }

    const QString address = m_addressEdit->text().trimmed();
    if (address.isEmpty()) {
        m_output->appendError(tr("Please enter a network address.\n"));
        return;
    }

    m_output->clear();
    m_output->appendNotice(tr("Tracing route to %1, 30 hops max:\n").arg(address));
    onStarted();
    m_tracer->start(address);
}

void TracerouteTab::onStarted() {
    m_actionButton->setText(tr("Stop"));
    m_addressEdit->setEnabled(false);
}

void TracerouteTab::onFinished() {
    m_actionButton->setText(tr("Trace"));
    m_addressEdit->setEnabled(true);
}

void TracerouteTab::onHopResult(int ttl, const QString &address, int rttMs) {
    if (address.isEmpty()) {
        m_output->appendPlain(tr("%1  *\n").arg(ttl, 2));
    } else {
        m_output->appendPlain(tr("%1  %2  %3 ms\n").arg(ttl, 2).arg(address).arg(rttMs));
    }
}

void TracerouteTab::onReachedDestination() {
    m_output->appendNotice(tr("Destination reached.\n"));
}

void TracerouteTab::onErrorOccurred(const QString &message) {
    m_output->appendError(message + QStringLiteral("\n"));
}
