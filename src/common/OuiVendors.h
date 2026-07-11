#pragma once

#include <QString>

// Best-effort MAC vendor lookup against a curated subset of the IEEE OUI
// registry (see OuiVendors.cpp) - not exhaustive, so unknown prefixes
// return an empty string.
namespace OuiVendors {

QString lookup(const QString &macAddress);

} // namespace OuiVendors
