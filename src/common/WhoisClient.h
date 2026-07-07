#pragma once

#include <QObject>
#include <QString>

class QTcpSocket;

// Speaks the whois protocol (RFC 3912) directly over a TCP socket on port
// 43, so lookups work without an external `whois` binary. In "automatic"
// mode it starts at IANA's root server and follows one referral to reach
// the authoritative registry, mirroring what command-line whois clients do.
class WhoisClient : public QObject {
    Q_OBJECT

public:
    explicit WhoisClient(QObject *parent = nullptr);

    // If server is empty, queries whois.iana.org and follows a referral.
    void query(const QString &target, const QString &server = QString());
    void abort();
    bool isRunning() const;

signals:
    void serverQueried(const QString &server);
    void chunkReceived(const QString &text);
    void finished();
    void errorOccurred(const QString &message);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onSocketError();

private:
    void connectToServer(const QString &server, const QString &queryText);
    static QString bootstrapQueryFor(const QString &target);
    static QString extractReferral(const QString &response);

    QTcpSocket *m_socket;
    QString m_target;
    QString m_pendingQuery;
    QString m_currentServer;
    QByteArray m_responseBuffer;
    int m_referralsLeft = 0;
    bool m_followReferrals = false;
};