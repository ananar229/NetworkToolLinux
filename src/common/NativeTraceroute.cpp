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
constexpr quint8 kIcmpEchoRequest = 8;
constexpr quint8 kIcmpEchoReply = 0;
constexpr quint16 kTcpProbePort = 443;

// Matches the wire layout of the standard ICMP echo header (type, code,
// checksum, identifier, sequence) — defined locally to avoid pulling in
// <netinet/ip_icmp.h>/<linux/icmp.h>, which don't mix cleanly with
// <linux/errqueue.h> in the same translation unit.
struct IcmpEchoHeader {
    quint8 type;
    quint8 code;
    quint16 checksum;
    quint16 id;
    quint16 sequence;
};

quint16 internetChecksum(const void *data, size_t length) {
    const auto *bytes = static_cast<const quint8 *>(data);
    quint32 sum = 0;
    while (length > 1) {
        sum += (static_cast<quint16>(bytes[0]) << 8) | bytes[1];
        bytes += 2;
        length -= 2;
    }
    if (length == 1) {
        sum += static_cast<quint16>(bytes[0]) << 8;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return static_cast<quint16>(~sum);
}

// Extracts the offending router's address and ICMP type from a socket
// error-queue entry. Returns false if nothing was queued.
bool readIcmpError(int fd, QString *offenderAddress, int *icmpType) {
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

    if (::recvmsg(fd, &msg, MSG_ERRQUEUE) < 0) {
        return false;
    }

    for (cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level != IPPROTO_IP || cmsg->cmsg_type != IP_RECVERR) {
            continue;
        }
        auto *ee = reinterpret_cast<sock_extended_err *>(CMSG_DATA(cmsg));
        if (ee->ee_origin == SO_EE_ORIGIN_ICMP) {
            *icmpType = ee->ee_type;
            auto *offender = reinterpret_cast<sockaddr_in *>(SO_EE_OFFENDER(ee));
            *offenderAddress = QHostAddress(ntohl(offender->sin_addr.s_addr)).toString();
        }
    }
    return true;
}

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
    m_mode = Mode::Icmp;
    m_anyHopResolved = false;

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
    if (m_mode == Mode::Icmp) {
        sendIcmpProbe();
    } else {
        sendTcpProbe();
    }
}

void NativeTraceroute::sendIcmpProbe() {
    if (m_fd < 0) {
        m_fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
        if (m_fd < 0) {
            emit errorOccurred(
                tr("Could not create UDP socket: %1").arg(QString::fromLocal8Bit(strerror(errno))));
            m_running = false;
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
    }

    ::setsockopt(m_fd, IPPROTO_IP, IP_TTL, &m_ttl, sizeof(m_ttl));

    struct {
        IcmpEchoHeader header;
        char payload[8];
    } packet{};
    packet.header.type = kIcmpEchoRequest;
    packet.header.code = 0;
    packet.header.checksum = 0;
    packet.header.id = htons(1);
    packet.header.sequence = htons(static_cast<quint16>(m_ttl));
    memcpy(packet.payload, "NetTrace", sizeof(packet.payload));
    packet.header.checksum = htons(internetChecksum(&packet, sizeof(packet)));

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = htonl(m_destination.toIPv4Address());

    m_probeTimer.start();
    ::sendto(m_fd, &packet, sizeof(packet), 0, reinterpret_cast<sockaddr *>(&dest), sizeof(dest));

    m_timeoutTimer->start(m_timeoutMs);
}

void NativeTraceroute::sendTcpProbe() {
    closeProbeSocket();

    m_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_fd < 0) {
        emit hopResult(m_ttl, QString(), -1);
        advanceOrFinish();
        return;
    }
    const int one = 1;
    ::setsockopt(m_fd, IPPROTO_IP, IP_RECVERR, &one, sizeof(one));
    ::setsockopt(m_fd, IPPROTO_IP, IP_TTL, &m_ttl, sizeof(m_ttl));
    const int flags = ::fcntl(m_fd, F_GETFL, 0);
    ::fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);

    m_readNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_readNotifier, &QSocketNotifier::activated, this, &NativeTraceroute::onSocketActivity);
    m_writeNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Write, this);
    connect(m_writeNotifier, &QSocketNotifier::activated, this, &NativeTraceroute::onSocketActivity);
    m_exceptionNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Exception, this);
    connect(m_exceptionNotifier, &QSocketNotifier::activated, this, &NativeTraceroute::onSocketActivity);

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(kTcpProbePort);
    dest.sin_addr.s_addr = htonl(m_destination.toIPv4Address());

    m_probeTimer.start();
    ::connect(m_fd, reinterpret_cast<sockaddr *>(&dest), sizeof(dest)); // expected: EINPROGRESS

    m_timeoutTimer->start(m_timeoutMs);
}

void NativeTraceroute::onSocketActivity() {
    if (!m_running) {
        return;
    }
    if (m_mode == Mode::Icmp) {
        onIcmpActivity();
    } else {
        onTcpActivity();
    }
}

void NativeTraceroute::onIcmpActivity() {
    QString offenderAddress;
    int icmpType = -1;
    if (readIcmpError(m_fd, &offenderAddress, &icmpType)) {
        const qint64 elapsedMs = m_probeTimer.elapsed();
        m_timeoutTimer->stop();

        if (offenderAddress.isEmpty()) {
            emit hopResult(m_ttl, QString(), -1);
            advanceOrFinish();
            return;
        }

        m_anyHopResolved = true;
        emit hopResult(m_ttl, offenderAddress, static_cast<int>(elapsedMs));
        if (icmpType == kIcmpDestUnreachable) {
            m_running = false;
            closeSocket();
            emit reachedDestination();
            emit finished();
            return;
        }
        advanceOrFinish();
        return;
    }

    // No error queued; check for a genuine Echo Reply, which means this
    // probe made it all the way to the destination and back.
    char replyBuffer[512];
    sockaddr_in replyFrom{};
    socklen_t replyFromLen = sizeof(replyFrom);
    const ssize_t replyLen = ::recvfrom(m_fd, replyBuffer, sizeof(replyBuffer), 0,
                                         reinterpret_cast<sockaddr *>(&replyFrom), &replyFromLen);
    if (replyLen < static_cast<ssize_t>(sizeof(IcmpEchoHeader))) {
        return; // spurious wakeup; keep waiting
    }

    const auto *reply = reinterpret_cast<IcmpEchoHeader *>(replyBuffer);
    if (reply->type != kIcmpEchoReply || ntohs(reply->sequence) != static_cast<quint16>(m_ttl)) {
        return;
    }

    const qint64 elapsedMs = m_probeTimer.elapsed();
    m_timeoutTimer->stop();
    const QString address = QHostAddress(ntohl(replyFrom.sin_addr.s_addr)).toString();
    m_anyHopResolved = true;
    emit hopResult(m_ttl, address, static_cast<int>(elapsedMs));
    m_running = false;
    closeSocket();
    emit reachedDestination();
    emit finished();
}

void NativeTraceroute::onTcpActivity() {
    QString offenderAddress;
    int icmpType = -1;
    if (readIcmpError(m_fd, &offenderAddress, &icmpType)) {
        const qint64 elapsedMs = m_probeTimer.elapsed();
        m_timeoutTimer->stop();

        if (offenderAddress.isEmpty()) {
            emit hopResult(m_ttl, QString(), -1);
            advanceOrFinish();
            return;
        }

        m_anyHopResolved = true;
        emit hopResult(m_ttl, offenderAddress, static_cast<int>(elapsedMs));
        if (icmpType == kIcmpDestUnreachable) {
            // Routing dead-end before ever reaching the target; stop rather
            // than loop, since the same error will repeat at every TTL.
            m_running = false;
            closeSocket();
            emit finished();
            return;
        }
        advanceOrFinish();
        return;
    }

    // A non-blocking connect() only finalizes (succeeds or fails) once the
    // fd becomes writable; ignore read/exception-only wakeups here.
    if (sender() != m_writeNotifier) {
        return;
    }

    int socketError = 0;
    socklen_t socketErrorLen = sizeof(socketError);
    ::getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &socketError, &socketErrorLen);

    const qint64 elapsedMs = m_probeTimer.elapsed();
    m_timeoutTimer->stop();

    if (socketError == 0 || socketError == ECONNREFUSED) {
        // A completed SYN-ACK handshake or an RST both prove the packet
        // reached the destination itself.
        m_anyHopResolved = true;
        emit hopResult(m_ttl, m_destination.toString(), static_cast<int>(elapsedMs));
        m_running = false;
        closeSocket();
        emit reachedDestination();
        emit finished();
        return;
    }

    emit hopResult(m_ttl, QString(), -1);
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
        if (m_mode == Mode::Icmp && !m_anyHopResolved) {
            m_mode = Mode::Tcp;
            m_ttl = 1;
            closeProbeSocket();
            emit fallingBackToTcp();
            sendProbe();
            return;
        }
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

void NativeTraceroute::closeProbeSocket() {
    delete m_readNotifier;
    m_readNotifier = nullptr;
    delete m_writeNotifier;
    m_writeNotifier = nullptr;
    delete m_exceptionNotifier;
    m_exceptionNotifier = nullptr;
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

void NativeTraceroute::closeSocket() {
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
    }
    closeProbeSocket();
}
