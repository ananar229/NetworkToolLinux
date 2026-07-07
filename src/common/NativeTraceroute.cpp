#include "NativeTraceroute.h"

#include <QHostInfo>
#include <QSocketNotifier>
#include <QTimer>

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/errqueue.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
constexpr int kIcmpDestUnreachable = 3;
constexpr int kIcmpTimeExceeded = 11;
} // namespace

NativeTraceroute::NativeTraceroute(QObject *parent) : QObject(parent) {}

NativeTraceroute::~NativeTraceroute() {
    closeSocket();
}

void NativeTraceroute::start(const QString &host, int maxHops, int timeoutMs) {
    if (m_running) {
        stop();
    }
    m_maxHops = maxHops;
    m_timeoutMs = timeoutMs;
    m_ttl = 1;

    const QHostAddress direct(host);
    if (!direct.isNull() && direct.protocol() == QAbstractSocket::IPv4Protocol) {
        beginWithAddress(direct);
        return;
    }

    QHostInfo::lookupHost(host, this, [this](const QHostInfo &info) {
        if (info.error() != QHostInfo::NoError) {
            emit errorOccurred(tr("Could not resolve host: %1").arg(info.errorString()));
            emit finished();
            return;
        }
        QHostAddress found;
        for (const QHostAddress &addr : info.addresses()) {
            if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
                found = addr;
                break;
            }
        }
        if (found.isNull()) {
            emit errorOccurred(tr("No IPv4 address found for that host (IPv6 traceroute is not supported)."));
            emit finished();
            return;
        }
        beginWithAddress(found);
    });
}

void NativeTraceroute::beginWithAddress(const QHostAddress &address) {
    m_destination = address;

    m_fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_fd < 0) {
        emit errorOccurred(tr("Could not create UDP socket: %1").arg(QString::fromLocal8Bit(strerror(errno))));
        emit finished();
        return;
    }

    const int one = 1;
    ::setsockopt(m_fd, IPPROTO_IP, IP_RECVERR, &one, sizeof(one));

    const int flags = ::fcntl(m_fd, F_GETFL, 0);
    ::fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);

    m_readNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_readNotifier, &QSocketNotifier::activated, this, &NativeTraceroute::onSocketActivity);
    m_exceptionNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Exception, this);
    connect(m_exceptionNotifier, &QSocketNotifier::activated, this, &NativeTraceroute::onSocketActivity);

    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &NativeTraceroute::onTimeout);

    m_running = true;
    sendProbe();
}

void NativeTraceroute::sendProbe() {
    if (!m_running) {
        return;
    }

    ::setsockopt(m_fd, IPPROTO_IP, IP_TTL, &m_ttl, sizeof(m_ttl));

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(m_basePort + static_cast<quint16>(m_ttl));
    dest.sin_addr.s_addr = htonl(m_destination.toIPv4Address());

    static const char payload[] = "NetworkTool-traceroute";
    m_probeTimer.start();
    ::sendto(m_fd, payload, sizeof(payload), 0, reinterpret_cast<sockaddr *>(&dest), sizeof(dest));

    m_timeoutTimer->start(m_timeoutMs);
}

void NativeTraceroute::onSocketActivity() {
    if (!m_running) {
        return;
    }

    char buffer[512];
    char control[512];

    iovec iov{};
    iov.iov_base = buffer;
    iov.iov_len = sizeof(buffer);

    sockaddr_in from{};
    msghdr msg{};
    msg.msg_name = &from;
    msg.msg_namelen = sizeof(from);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    const ssize_t n = ::recvmsg(m_fd, &msg, MSG_ERRQUEUE);
    if (n < 0) {
        // Spurious notifier wakeup with nothing queued yet; wait for the real one.
        return;
    }

    const qint64 elapsedMs = m_probeTimer.elapsed();
    m_timeoutTimer->stop();

    QString offenderAddress;
    int icmpType = -1;

    for (cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level != IPPROTO_IP || cmsg->cmsg_type != IP_RECVERR) {
            continue;
        }
        auto *ee = reinterpret_cast<sock_extended_err *>(CMSG_DATA(cmsg));
        if (ee->ee_origin == SO_EE_ORIGIN_ICMP) {
            icmpType = ee->ee_type;
            auto *offender = reinterpret_cast<sockaddr_in *>(SO_EE_OFFENDER(ee));
            offenderAddress = QHostAddress(ntohl(offender->sin_addr.s_addr)).toString();
        } else if (ee->ee_origin == SO_EE_ORIGIN_LOCAL) {
            emit errorOccurred(tr("Local network error while probing hop %1.").arg(m_ttl));
        }
    }

    if (offenderAddress.isEmpty()) {
        emit hopResult(m_ttl, QString(), -1);
        advanceOrFinish();
        return;
    }

    emit hopResult(m_ttl, offenderAddress, static_cast<int>(elapsedMs));
    if (icmpType == kIcmpDestUnreachable) {
        m_running = false;
        closeSocket();
        emit reachedDestination();
        emit finished();
        return;
    }
    if (icmpType == kIcmpTimeExceeded) {
        advanceOrFinish();
        return;
    }
    advanceOrFinish();
}

void NativeTraceroute::onTimeout() {
    if (!m_running) {
        return;
    }
    emit hopResult(m_ttl, QString(), -1);
    advanceOrFinish();
}

void NativeTraceroute::advanceOrFinish() {
    m_ttl++;
    if (m_ttl > m_maxHops) {
        m_running = false;
        closeSocket();
        emit finished();
        return;
    }
    sendProbe();
}

void NativeTraceroute::stop() {
    if (!m_running) {
        return;
    }
    m_running = false;
    closeSocket();
    emit finished();
}

void NativeTraceroute::closeSocket() {
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
    }
    delete m_readNotifier;
    m_readNotifier = nullptr;
    delete m_exceptionNotifier;
    m_exceptionNotifier = nullptr;
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}
