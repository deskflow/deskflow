/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "NetworkMonitor.h"

#include <QAbstractSocket>
#include <QList>
#include <QNetworkInterface>
#include <QSet>
#include <QTimer>

namespace deskflow::gui {

bool NetworkMonitor::isVirtualInterface(const QString &interfaceName) const
{
  // Common virtual network interface patterns
  static const QStringList virtualPatterns = {
      QStringLiteral("vboxnet"), // VirtualBox host-only networks
      QStringLiteral("vmnet"),   // VMware virtual networks
      QStringLiteral("docker"),  // Docker bridge networks
      QStringLiteral("virbr"),   // libvirt bridge networks
      QStringLiteral("veth"),    // Virtual ethernet
      QStringLiteral("br-"),     // Bridge interfaces (some are virtual)
      QStringLiteral("tun"),     // Tunnel interfaces
      QStringLiteral("tap"),     // TAP interfaces
      QStringLiteral("utun"),    // User tunnel (macOS)
      QStringLiteral("awdl"),    // Apple Wireless Direct Link
      QStringLiteral("p2p"),     // Peer-to-peer
      QStringLiteral("llw"),     // Link-local wireless
      QStringLiteral("anpi"),    // Apple network interface
  };

  for (const auto &pattern : virtualPatterns) {
    if (pattern.compare(interfaceName, Qt::CaseInsensitive) == 0) {
      return true;
    }
  }
  return false;
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

    const bool isVirtual = isVirtualInterface(interface.name());
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

  // Helper lambda to check if IP is in private RFC1918 range (Qt 6.4 compatible alternative to isPrivateUse())
  auto isPrivateAddress = [](const QHostAddress &addr) -> bool {
    quint32 ip = addr.toIPv4Address();
    // 10.0.0.0/8
    if ((ip & 0xFF000000) == 0x0A000000) return true;
    // 172.16.0.0/12
    if ((ip & 0xFFF00000) == 0xAC100000) return true;
    // 192.168.0.0/16
    if ((ip & 0xFFFF0000) == 0xC0A80000) return true;
    return false;
  };

  std::ranges::sort(physicalIPs, [&isPrivateAddress](const QHostAddress &a, const QHostAddress &b) {
    if (isPrivateAddress(a) != isPrivateAddress(b))
      return true;
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
