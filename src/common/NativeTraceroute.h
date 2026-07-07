#pragma once

#include <QElapsedTimer>
#include <QHostAddress>
#include <QObject>
#include <QString>

class QSocketNotifier;
class QTimer;

// Traces the route to a host without any external `traceroute` binary and
// without root/CAP_NET_RAW: it sends UDP probes with increasing TTL and
// reads the resulting ICMP Time Exceeded / Destination Unreachable errors
// off the socket's error queue (IP_RECVERR + MSG_ERRQUEUE), the same
// unprivileged technique Linux's `tracepath` uses. Linux only, IPv4 only.
class NativeTraceroute : public QObject {
    Q_OBJECT

public:
    explicit NativeTraceroute(QObject *parent = nullptr);
    ~NativeTraceroute() override;

    void start(const QString &host, int maxHops = 30, int timeoutMs = 2000);
    void stop();
    bool isRunning() const { return m_running; }

signals:
    // address is empty when the hop timed out.
    void hopResult(int ttl, const QString &address, int rttMs);
    void reachedDestination();
    void finished();
    void errorOccurred(const QString &message);

private slots:
    void onSocketActivity();
    void onTimeout();

private:
    void beginWithAddress(const QHostAddress &address);
    void sendProbe();
    void advanceOrFinish();
    void closeSocket();

    int m_fd = -1;
    QSocketNotifier *m_readNotifier = nullptr;
    QSocketNotifier *m_exceptionNotifier = nullptr;
    QTimer *m_timeoutTimer = nullptr;
    QElapsedTimer m_probeTimer;
    QHostAddress m_destination;
    quint16 m_basePort = 33434;
    int m_ttl = 1;
    int m_maxHops = 30;
    int m_timeoutMs = 2000;
    bool m_running = false;
};
