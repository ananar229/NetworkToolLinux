#pragma once

#include <QString>

// Formats netstat-style reports straight from the Linux /proc/net
// pseudo-files, so no `netstat`/`ip`/`ss` binary is required at all.
namespace ProcNetInfo {

QString routingTable();
QString protocolStatistics();
QString multicastGroups();

} // namespace ProcNetInfo
