#include "ProcNetInfo.h"

#include <QFile>
#include <QHostAddress>
#include <QRegularExpression>
#include <QTextStream>

namespace {

// /proc files report size() == 0 and QTextStream::atEnd() can return true
// before any read has happened, so line-by-line QTextStream iteration
// silently reads nothing. Reading the whole pseudo-file in one shot and
// splitting it ourselves sidesteps that.
QStringList readAllLines(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
}

QString ipv4FromHex(const QString &hex) {
    bool ok = false;
    const quint32 val = hex.toUInt(&ok, 16);
    if (!ok) {
        return QStringLiteral("?");
    }
    // /proc/net stores IPv4 addresses as a little-endian hex dump of the
    // in-memory (network-order) bytes, so the low byte of the parsed value
    // is actually the first dotted-decimal octet: byte-swap it back into
    // the big-endian form QHostAddress(quint32) expects.
    const quint32 hostOrder = ((val & 0x000000FFu) << 24) | ((val & 0x0000FF00u) << 8) |
                               ((val & 0x00FF0000u) >> 8) | ((val & 0xFF000000u) >> 24);
    return QHostAddress(hostOrder).toString();
}

QString ipv6FromHex(const QString &hex) {
    if (hex.size() != 32) {
        return QStringLiteral("?");
    }
    Q_IPV6ADDR addr;
    for (int i = 0; i < 16; ++i) {
        addr[i] = static_cast<quint8>(hex.mid(i * 2, 2).toUInt(nullptr, 16));
    }
    return QHostAddress(addr).toString();
}

struct StatGroup {
    QString name;
    QVector<QPair<QString, QString>> fields;
};

QVector<StatGroup> parseSnmpStyleFile(const QString &path) {
    QVector<StatGroup> groups;
    const QStringList lines = readAllLines(path);

    for (int i = 0; i + 1 < lines.size(); i += 2) {
        const QString &headerLine = lines[i];
        const QString &valueLine = lines[i + 1];

        const int headerColon = headerLine.indexOf(':');
        const int valueColon = valueLine.indexOf(':');
        if (headerColon < 0 || valueColon < 0) {
            continue;
        }

        StatGroup group;
        group.name = headerLine.left(headerColon);
        const QStringList keys = headerLine.mid(headerColon + 1).split(' ', Qt::SkipEmptyParts);
        const QStringList values = valueLine.mid(valueColon + 1).split(' ', Qt::SkipEmptyParts);
        for (int k = 0; k < keys.size() && k < values.size(); ++k) {
            group.fields.append({keys[k], values[k]});
        }
        groups.append(group);
    }
    return groups;
}

} // namespace

QString ProcNetInfo::routingTable() {
    QString result;
    QTextStream out(&result);

    const QStringList routeLines = readAllLines(QStringLiteral("/proc/net/route"));
    if (!routeLines.isEmpty()) {
        out << QStringLiteral("Kernel IPv4 routing table\n");
        out << QString("%1  %2  %3  %4  %5\n")
                   .arg(QStringLiteral("Destination"), -18)
                   .arg(QStringLiteral("Gateway"), -18)
                   .arg(QStringLiteral("Genmask"), -18)
                   .arg(QStringLiteral("Flags"), -6)
                   .arg(QStringLiteral("Iface"));
        for (int i = 1; i < routeLines.size(); ++i) { // skip header line
            const QString &line = routeLines[i];
            if (line.trimmed().isEmpty()) {
                continue;
            }
            const QStringList f = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (f.size() < 8) {
                continue;
            }
            const QString iface = f[0];
            const QString dest = ipv4FromHex(f[1]);
            const QString gw = ipv4FromHex(f[2]);
            const quint32 flagsVal = f[3].toUInt(nullptr, 16);
            const QString mask = ipv4FromHex(f[7]);

            QString flags;
            if (flagsVal & 0x1) flags += QLatin1Char('U');
            if (flagsVal & 0x2) flags += QLatin1Char('G');
            if (flagsVal & 0x4) flags += QLatin1Char('H');

            out << QString("%1  %2  %3  %4  %5\n")
                       .arg(dest == QStringLiteral("0.0.0.0") ? QStringLiteral("default") : dest, -18)
                       .arg(gw, -18)
                       .arg(mask, -18)
                       .arg(flags, -6)
                       .arg(iface);
        }
    } else {
        out << QStringLiteral("Could not read /proc/net/route.\n");
    }

    out << QStringLiteral("\n");

    const QStringList route6Lines = readAllLines(QStringLiteral("/proc/net/ipv6_route"));
    if (!route6Lines.isEmpty()) {
        out << QStringLiteral("Kernel IPv6 routing table\n");
        out << QString("%1  %2  %3\n")
                   .arg(QStringLiteral("Destination"), -42)
                   .arg(QStringLiteral("Next Hop"), -42)
                   .arg(QStringLiteral("Iface"));
        for (const QString &line : route6Lines) {
            if (line.trimmed().isEmpty()) {
                continue;
            }
            const QStringList f = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (f.size() < 10) {
                continue;
            }
            const QString dest = ipv6FromHex(f[0]);
            const int destPrefix = f[1].toInt(nullptr, 16);
            const QString nextHop = ipv6FromHex(f[4]);
            const QString iface = f[9];

            const QString destStr = (dest == QStringLiteral("::") && destPrefix == 0)
                                         ? QStringLiteral("default")
                                         : QStringLiteral("%1/%2").arg(dest).arg(destPrefix);
            out << destStr.leftJustified(42) << QStringLiteral("  ") << nextHop.leftJustified(42)
                << QStringLiteral("  ") << iface << QStringLiteral("\n");
        }
    }

    return result;
}

QString ProcNetInfo::protocolStatistics() {
    QString result;
    QTextStream out(&result);

    for (const QString &path : {QStringLiteral("/proc/net/snmp"), QStringLiteral("/proc/net/netstat")}) {
        const QVector<StatGroup> groups = parseSnmpStyleFile(path);
        for (const StatGroup &group : groups) {
            QString body;
            QTextStream bodyStream(&body);
            for (const auto &field : group.fields) {
                if (field.second == QStringLiteral("0")) {
                    continue;
                }
                bodyStream << QStringLiteral("  ") << field.first << QStringLiteral(": ") << field.second
                           << QStringLiteral("\n");
            }
            if (!body.isEmpty()) {
                out << group.name << QStringLiteral(":\n") << body;
            }
        }
    }

    if (result.isEmpty()) {
        return QStringLiteral("No protocol statistics available.\n");
    }
    return result;
}

QString ProcNetInfo::multicastGroups() {
    QString result;
    QTextStream out(&result);
    bool any = false;

    const QStringList igmpLines = readAllLines(QStringLiteral("/proc/net/igmp"));
    {
        QString currentDevice;
        for (int i = 1; i < igmpLines.size(); ++i) { // skip header line
            const QString &rawLine = igmpLines[i];
            if (rawLine.isEmpty()) {
                continue;
            }
            if (!rawLine.at(0).isSpace()) {
                const int colon = rawLine.indexOf(':');
                if (colon < 0) {
                    continue;
                }
                const QStringList left = rawLine.left(colon).split(QLatin1Char('\t'));
                currentDevice = left.size() >= 2 ? left[1].trimmed() : rawLine.left(colon).trimmed();
                out << QStringLiteral("Interface ") << currentDevice << QStringLiteral(" (IPv4):\n");
                any = true;
            } else {
                const QStringList tokens = rawLine.trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (!tokens.isEmpty()) {
                    out << QStringLiteral("  ") << ipv4FromHex(tokens[0]) << QStringLiteral("\n");
                }
            }
        }
    }

    const QStringList igmp6Lines = readAllLines(QStringLiteral("/proc/net/igmp6"));
    {
        QString currentDevice;
        for (const QString &line : igmp6Lines) {
            if (line.trimmed().isEmpty()) {
                continue;
            }
            const QStringList tokens = line.trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (tokens.size() < 3) {
                continue;
            }
            const QString device = tokens[1];
            if (device != currentDevice) {
                currentDevice = device;
                out << QStringLiteral("Interface ") << currentDevice << QStringLiteral(" (IPv6):\n");
                any = true;
            }
            out << QStringLiteral("  ") << ipv6FromHex(tokens[2]) << QStringLiteral("\n");
        }
    }

    if (!any) {
        return QStringLiteral("No multicast group memberships found.\n");
    }
    return result;
}
