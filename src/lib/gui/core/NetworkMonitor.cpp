/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "NetworkMonitor.h"

#include <QAbstractSocket>
#include <QList>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>

namespace deskflow::gui {

namespace {

bool isPrivateUseAddress(const QHostAddress &address)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  return address.isPrivateUse();
#else
  if (address.protocol() != QAbstractSocket::IPv4Protocol) {
    return false;
  }

  const auto ipv4 = address.toIPv4Address();
  return (ipv4 & 0xff000000u) == 0x0a000000u ||     // 10.0.0.0/8
         (ipv4 & 0xfff00000u) == 0xac100000u ||     // 172.16.0.0/12
         (ipv4 & 0xffff0000u) == 0xc0a80000u;       // 192.168.0.0/16
#endif
}

} // namespace

bool NetworkMonitor::isVirtualInterface(const QString &interfaceName)
{
  // Common virtual network interface patterns
  static const auto virtualRegEx = QRegularExpression(
      QStringLiteral("^vboxnet|vmnet|docker|virbr|veth|br\\-|tun|utun|awdl|p2p|llw|anpi|tap"),
      QRegularExpression::CaseInsensitiveOption
  );
  return virtualRegEx.match(interfaceName).hasMatch();
}

NetworkMonitor::NetworkMonitor(QObject *parent) : QObject(parent), m_checkTimer(new QTimer(this))
{
  connect(m_checkTimer, &QTimer::timeout, this, &NetworkMonitor::updateNetworkState);
}

void NetworkMonitor::startMonitoring(int intervalMs)
{
  if (m_isMonitoring) {
    return;
  }

  updateNetworkState();

  m_checkTimer->start(intervalMs);
  m_isMonitoring = true;
}

void NetworkMonitor::stopMonitoring()
{
  if (!m_isMonitoring) {
    return;
  }

  m_checkTimer->stop();
  m_isMonitoring = false;
}

QStringList NetworkMonitor::getAvailableIPv4Addresses() const
{
  QList<QHostAddress> physicalIPs;
  QList<QHostAddress> virtualIPs;
  QSet<QHostAddress> uniqueAddresses;

  const auto allInterfaces = QNetworkInterface::allInterfaces();
  for (const auto &interface : allInterfaces) {
    if (!(interface.flags() & QNetworkInterface::IsUp) || !(interface.flags() & QNetworkInterface::IsRunning) ||
        (interface.flags() & QNetworkInterface::IsLoopBack)) {
      continue;
    }

    const bool isP2P = (interface.flags() & QNetworkInterface::IsPointToPoint);
    const bool isVirtual = isVirtualInterface(interface.name()) || isP2P;
    const auto addressEntries = interface.addressEntries();

    for (const auto &entry : addressEntries) {
      const QHostAddress address = entry.ip();

      if (address.protocol() != QAbstractSocket::IPv4Protocol || address.isLinkLocal() || address.isLoopback() ||
          uniqueAddresses.contains(address)) {
        continue;
      }

      uniqueAddresses.insert(address);

      if (isVirtual) {
        virtualIPs.append(address);
      } else {
        physicalIPs.append(address);
      }
    }
  }

  std::ranges::sort(physicalIPs, [](const QHostAddress &a, const QHostAddress &b) {
    if (isPrivateUseAddress(a) != isPrivateUseAddress(b))
      return isPrivateUseAddress(a);
    return a.toIPv4Address() < b.toIPv4Address();
  });

  std::ranges::sort(virtualIPs, [](const QHostAddress &a, const QHostAddress &b) {
    return a.toIPv4Address() < b.toIPv4Address();
  });

  auto result = physicalIPs;
  result.append(virtualIPs);

  QStringList ipList;
  for (const auto &host : result) {
    ipList.append(host.toString());
  }
  return ipList;
}

void NetworkMonitor::setIpAddresses(const QStringList &newAddresses)
{
  if (newAddresses == m_lastAddresses)
    return;
  m_lastAddresses = newAddresses;
  Q_EMIT ipAddressesChanged(m_lastAddresses);
}

void NetworkMonitor::updateNetworkState()
{
  setIpAddresses(getAvailableIPv4Addresses());
}

} // namespace deskflow::gui
