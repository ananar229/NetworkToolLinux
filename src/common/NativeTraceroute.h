#pragma once

#include <QElapsedTimer>
#include <QHostAddress>
#include <QObject>
#include <QString>

class QSocketNotifier;
class QTimer;

// Traces the route to a host without any external `traceroute` binary and
// without root/CAP_NET_RAW.
//
// Primary technique: ICMP Echo Request probes (via Linux's unprivileged
// "ping socket", SOCK_DGRAM+IPPROTO_ICMP) with increasing TTL. Intermediate
// hops are read off the socket's error queue (IP_RECVERR + MSG_ERRQUEUE,
// same technique `tracepath` uses); the destination is detected by an
// actual Echo Reply coming back.
//
// Fallback: some hosts/firewalls drop ICMP entirely (ping never gets a
// reply from them at all, even directly). If a full ICMP pass produces zero
// resolved hops, this automatically retries using TCP SYN probes to port
// 443 instead — a fresh non-blocking connect() per TTL, again reading
// intermediate ICMP errors via IP_RECVERR, with a completed connect
// (accepted or refused) marking the destination as reached. Networks that
// filter ICMP commonly still allow inbound TCP:443.
//
// Linux only, IPv4 only.
class NativeTraceroute : public QObject {
    Q_OBJECT

public:
    explicit NativeTraceroute(QObject *parent = nullptr);
    ~NativeTraceroute() override;

    void start(const QString &host, int maxHops = 64, int timeoutMs = 2000);
    void stop();
    bool isRunning() const { return m_running; }

signals:
    // address is empty when the hop timed out.
    void hopResult(int ttl, const QString &address, int rttMs);
    void reachedDestination();
    void fallingBackToTcp();
    void finished();
    void errorOccurred(const QString &message);

private slots:
    void onSocketActivity();
    void onTimeout();

private:
    enum class Mode { Icmp, Tcp };

    void beginWithAddress(const QHostAddress &address);
    void sendProbe();
    void sendIcmpProbe();
    void sendTcpProbe();
    void onIcmpActivity();
    void onTcpActivity();
    void advanceOrFinish();
    void closeProbeSocket();
    void closeSocket();

    Mode m_mode = Mode::Icmp;
    bool m_anyHopResolved = false;

    int m_fd = -1;
    QSocketNotifier *m_readNotifier = nullptr;
    QSocketNotifier *m_writeNotifier = nullptr;
    QSocketNotifier *m_exceptionNotifier = nullptr;
    QTimer *m_timeoutTimer = nullptr;
    QElapsedTimer m_probeTimer;
    QHostAddress m_destination;
    int m_ttl = 1;
    int m_maxHops = 64;
    int m_timeoutMs = 2000;
    bool m_running = false;
};
