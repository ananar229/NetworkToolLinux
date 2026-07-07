#include "NetstatTab.h"

#include "../common/OutputConsole.h"
#include "../common/ProcNetInfo.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

NetstatTab::NetstatTab(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *optionsBox = new QGroupBox();
    auto *optionsLayout = new QVBoxLayout(optionsBox);
    m_routingRadio = new QRadioButton(tr("Display routing table information"));
    m_statsRadio = new QRadioButton(tr("Display comprehensive network statistics for each protocol"));
    m_multicastRadio = new QRadioButton(tr("Display the list of multicast group memberships"));
    m_routingRadio->setChecked(true);
    optionsLayout->addWidget(m_routingRadio);
    optionsLayout->addWidget(m_statsRadio);
    optionsLayout->addWidget(m_multicastRadio);
    layout->addWidget(optionsBox);

    auto *buttonRow = new QHBoxLayout();
    m_runButton = new QPushButton(tr("Netstat"));
    m_runButton->setDefault(true);
    buttonRow->addStretch(1);
    buttonRow->addWidget(m_runButton);
    layout->addLayout(buttonRow);

    m_output = new OutputConsole();
    layout->addWidget(m_output, 1);

    connect(m_runButton, &QPushButton::clicked, this, &NetstatTab::run);
}

NetstatTab::Mode NetstatTab::currentMode() const {
    if (m_statsRadio->isChecked()) {
        return Mode::ProtocolStats;
    }
    if (m_multicastRadio->isChecked()) {
        return Mode::Multicast;
    }
    return Mode::Routing;
}

void NetstatTab::run() {
    m_output->clear();

    switch (currentMode()) {
        case Mode::Routing:
            m_output->appendPlain(ProcNetInfo::routingTable());
            break;
        case Mode::ProtocolStats:
            m_output->appendPlain(ProcNetInfo::protocolStatistics());
            break;
        case Mode::Multicast:
            m_output->appendPlain(ProcNetInfo::multicastGroups());
            break;
    }
}
