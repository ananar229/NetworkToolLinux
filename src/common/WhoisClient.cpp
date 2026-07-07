#include "WhoisClient.h"

#include <QHostAddress>
#include <QRegularExpression>
#include <QTcpSocket>

namespace {
constexpr quint16 kWhoisPort = 43;
constexpr int kMaxReferrals = 3;
}

WhoisClient::WhoisClient(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)) {
    connect(m_socket, &QTcpSocket::readyRead, this, &WhoisClient::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &WhoisClient::onDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &WhoisClient::onSocketError);
    connect(m_socket, &QTcpSocket::connected, this,
            [this] { m_socket->write((m_pendingQuery + QStringLiteral("\r\n")).toUtf8()); });
}

void WhoisClient::query(const QString &target, const QString &server) {
    if (isRunning()) {
        abort();
    }
    m_target = target;
    if (!server.isEmpty()) {
        m_followReferrals = false;
        m_referralsLeft = 0;
        connectToServer(server, target);
        return;
    }

    // IANA only hands back a referral when asked about the bare TLD (or an
    // IP/ASN, which it resolves directly); asking it about a full domain
    // name just returns the TLD's own registry record with no referral.
    m_followReferrals = true;
    m_referralsLeft = kMaxReferrals;
    connectToServer(QStringLiteral("whois.iana.org"), bootstrapQueryFor(target));
}

QString WhoisClient::bootstrapQueryFor(const QString &target) {
    if (!QHostAddress(target).isNull()) {
        return target;
    }
    const int lastDot = target.lastIndexOf(QLatin1Char('.'));
    if (lastDot < 0) {
        return target;
    }
    return target.mid(lastDot + 1);
}

void WhoisClient::connectToServer(const QString &server, const QString &queryText) {
    m_responseBuffer.clear();
    m_pendingQuery = queryText;
    m_currentServer = server;
    emit serverQueried(server);
    m_socket->connectToHost(server, kWhoisPort);
}

void WhoisClient::abort() {
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }
}

bool WhoisClient::isRunning() const {
    return m_socket->state() != QAbstractSocket::UnconnectedState;
}

void WhoisClient::onReadyRead() {
    m_responseBuffer += m_socket->readAll();
}

void WhoisClient::onDisconnected() {
    const QString response = QString::fromUtf8(m_responseBuffer);
    emit chunkReceived(response);

    if (m_followReferrals && m_referralsLeft > 0) {
        const QString referral = extractReferral(response);
        if (!referral.isEmpty() && referral.compare(m_currentServer, Qt::CaseInsensitive) != 0) {
            m_referralsLeft--;
            // Every hop past the IANA bootstrap query is asked about the
            // actual target, not the TLD used to find it.
            connectToServer(referral, m_target);
            return;
        }
    }

    emit finished();
}

void WhoisClient::onSocketError() {
    if (m_socket->error() == QAbstractSocket::RemoteHostClosedError) {
        return;
    }
    emit errorOccurred(m_socket->errorString());
    emit finished();
}

QString WhoisClient::extractReferral(const QString &response) {
    static const QRegularExpression pattern(
        QStringLiteral(R"(^\s*(?:refer|referralserver|whois|registrar\s+whois\s+server)\s*:\s*(?:whois://)?([A-Za-z0-9.\-]+))"),
        QRegularExpression::MultilineOption | QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = pattern.match(response);
    if (!match.hasMatch()) {
        return {};
    }
    return match.captured(1);
}
