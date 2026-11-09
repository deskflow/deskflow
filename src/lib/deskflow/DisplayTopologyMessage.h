// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "common/DisplayTopology.h"
namespace deskflow {
class IStream;
}
namespace deskflow::display {
inline constexpr auto topologyMessage = "DTOP%s%s";
// Advertise the extension only inside the existing mutually verified TLS path.
inline int offeredMinor(bool tls, bool verifyPeers)
{
  return tls && verifyPeers ? 9 : 8;
}
struct Message
{
  QString device;
  std::optional<Topology> topology;
  QByteArray workspace;
};
void writeTopologyMessage(IStream *, const QString &, const Topology &, const QByteArray &workspace = {});
// Code has already been consumed by the proxy dispatcher. Empty payload means
// unavailable, not one invented aggregate monitor. Invalid input disconnects.
Message readTopologyMessage(IStream *, const QString &expectedDevice = {});
} // namespace deskflow::display
