// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QHostAddress>
#include <QNetworkInterface>
#include <QString>
#include <QVector>
#include <functional>
namespace deskflow::gui {
// A selected, private IPv4 link. Index zero / system-wide discovery is forbidden.
// Deliberately IPv4-only until scoped IPv6 endpoints are verified on both OSes.
struct DiscoveryScope
{
  quint32 index = 0;
  QString name;
  QHostAddress address;
  int prefix = -1;
  bool operator==(const DiscoveryScope &) const = default;
  bool valid() const;
  static bool privateAddress(const QHostAddress &address);
  static bool eligibleLink(
      const QString &name, const QString &label, QNetworkInterface::InterfaceType type,
      QNetworkInterface::InterfaceFlags flags
  );
  static QVector<DiscoveryScope> current();
};
using DiscoveryScopeSource = std::function<QVector<DiscoveryScope>()>;
struct DiscoveryAdvertisement
{
  DiscoveryScope scope;
  QString token; // Random per exposure; never a machine, account or trusted-device identifier.
  int port = 0;
  QString instance() const
  {
    return QStringLiteral("Deskflow-") + token;
  }
  QString host() const
  {
    return QStringLiteral("deskflow-") + token + QStringLiteral(".local");
  }
  bool valid() const;
  static QString newToken();
};
inline constexpr int kDiscoverySessionMs = 5 * 60 * 1000;
} // namespace deskflow::gui
