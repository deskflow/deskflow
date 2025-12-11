/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
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

  return virtualPatterns.contains(interfaceName, Qt::CaseInsensitive);
}

NetworkMonitor::NetworkMonitor(QObject *parent) : QObject(parent), m_checkTimer(new QTimer(this)), m_isMonitoring(false)
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

QList<QHostAddress> NetworkMonitor::getAvailableIPv4Addresses() const
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

      if (address.protocol() != QAbstractSocket::IPv4Protocol ||
          address.isInSubnet(QHostAddress::parseSubnet(QStringLiteral("169.254/16"))) || address.isLoopback() ||
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

  auto physicalComparator = [](const QHostAddress &a, const QHostAddress &b) {
    static const auto localIpFilter = QStringLiteral("192.168/16");
    if (a.isInSubnet(QHostAddress::parseSubnet(localIpFilter)) !=
        b.isInSubnet(QHostAddress::parseSubnet(localIpFilter))) {
      return true;
    }
    return a.toIPv4Address() < b.toIPv4Address();
  };

  std::sort(physicalIPs.begin(), physicalIPs.end(), physicalComparator);

  std::sort(virtualIPs.begin(), virtualIPs.end(), [](const QHostAddress &a, const QHostAddress &b) {
    return a.toIPv4Address() < b.toIPv4Address();
  });

  auto result = physicalIPs;
  result.append(virtualIPs);

  return result;
}

QHostAddress NetworkMonitor::getSuggestedIPv4Address() const
{
  const auto addresses = getAvailableIPv4Addresses();
  if (addresses.isEmpty())
    return QHostAddress();
  return addresses.first();
}

void NetworkMonitor::setIpAddresses(const QList<QHostAddress> &newAddresses)
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
