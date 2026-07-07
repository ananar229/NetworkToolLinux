#include "FingerTab.h"

#include "../common/OutputConsole.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTcpSocket>
#include <QVBoxLayout>

namespace {
constexpr quint16 kFingerPort = 79;
}

FingerTab::FingerTab(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    layout->addWidget(new QLabel(tr("Enter a user@host to finger (or just a host to list its users):")));
    m_addressEdit = new QLineEdit();
    m_addressEdit->setPlaceholderText(tr("e.g. user@example.com"));
    layout->addWidget(m_addressEdit);

    auto *buttonRow = new QHBoxLayout();
    m_runButton = new QPushButton(tr("Finger"));
    m_runButton->setDefault(true);
    buttonRow->addStretch(1);
    buttonRow->addWidget(m_runButton);
    layout->addLayout(buttonRow);

    m_output = new OutputConsole();
    layout->addWidget(m_output, 1);

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &FingerTab::onConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &FingerTab::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &FingerTab::onFinished);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &FingerTab::onErrorOccurred);
    connect(m_runButton, &QPushButton::clicked, this, &FingerTab::run);
    connect(m_addressEdit, &QLineEdit::returnPressed, this, &FingerTab::run);
}

void FingerTab::run() {
    const QString input = m_addressEdit->text().trimmed();
    if (input.isEmpty()) {
        m_output->appendError(tr("Please enter a user@host address.\n"));
        return;
    }

    QString host;
    const int at = input.indexOf(QLatin1Char('@'));
    if (at >= 0) {
        m_user = input.left(at);
        host = input.mid(at + 1);
    } else {
        m_user.clear();
        host = input;
    }
    if (host.isEmpty()) {
        m_output->appendError(tr("Please include a host, e.g. user@example.com.\n"));
        return;
    }

    m_output->clear();
    m_runButton->setEnabled(false);
    m_output->appendNotice(tr("Connecting to %1:79 ...\n").arg(host));
    m_socket->connectToHost(host, kFingerPort);
}

void FingerTab::onConnected() {
    m_socket->write((m_user + QStringLiteral("\r\n")).toUtf8());
}

void FingerTab::onReadyRead() {
    m_output->appendPlain(QString::fromUtf8(m_socket->readAll()));
}

void FingerTab::onFinished() {
    m_runButton->setEnabled(true);
}

void FingerTab::onErrorOccurred() {
    if (m_socket->error() == QAbstractSocket::RemoteHostClosedError) {
        return;
    }
    m_output->appendError(tr("Finger query failed: %1\n").arg(m_socket->errorString()));
    onFinished();
}
