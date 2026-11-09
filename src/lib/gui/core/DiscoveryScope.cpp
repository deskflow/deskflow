// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DiscoveryScope.h"
#include <QRegularExpression>
#include <QUuid>
#ifdef Q_OS_MACOS
#include <SystemConfiguration/SystemConfiguration.h>
#endif
#ifdef Q_OS_WIN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <iphlpapi.h>
#include <netioapi.h>
#include <windows.h>
#include <winsock2.h>
#endif
namespace deskflow::gui {
namespace {
bool platformPhysical(const QNetworkInterface &network)
{
#ifdef Q_OS_MACOS
  const auto interfaces = SCNetworkInterfaceCopyAll();
  if (!interfaces)
    return false;
  bool physical = false;
  for (CFIndex i = 0; i < CFArrayGetCount(interfaces); ++i) {
    const auto interface = static_cast<SCNetworkInterfaceRef>(CFArrayGetValueAtIndex(interfaces, i));
    const auto type = SCNetworkInterfaceGetInterfaceType(interface), bsd = SCNetworkInterfaceGetBSDName(interface);
    char name[128]{};
    if (type && bsd && CFStringGetCString(bsd, name, sizeof(name), kCFStringEncodingUTF8) &&
        network.name() == QString::fromUtf8(name) &&
        (CFEqual(type, kSCNetworkInterfaceTypeEthernet) || CFEqual(type, kSCNetworkInterfaceTypeIEEE80211))) {
      physical = true;
      break;
    }
  }
  CFRelease(interfaces);
  return physical;
#elif defined(Q_OS_WIN)
  MIB_IFROW row{};
  row.dwIndex = static_cast<DWORD>(network.index());
  if (GetIfEntry(&row) != NO_ERROR)
    return false;
  return row.dwType == IF_TYPE_ETHERNET_CSMACD || row.dwType == IF_TYPE_IEEE80211;
#else
  Q_UNUSED(network);
  return false;
#endif
}
} // namespace

bool DiscoveryScope::privateAddress(const QHostAddress &address)
{
  if (address.protocol() != QAbstractSocket::IPv4Protocol)
    return false;
  const auto n = address.toIPv4Address();
  return (n & 0xff000000u) == 0x0a000000u || (n & 0xfff00000u) == 0xac100000u || (n & 0xffff0000u) == 0xc0a80000u ||
         (n & 0xffff0000u) == 0xa9fe0000u;
}
bool DiscoveryScope::valid() const
{
  if (index == 0 || index >= 0x80000000u || name.isEmpty() || name.size() > 128 || !privateAddress(address) ||
      prefix < 8 || prefix > 30)
    return false;
  const auto n = address.toIPv4Address();
  const int privatePrefix = (n & 0xff000000u) == 0x0a000000u ? 8 : (n & 0xfff00000u) == 0xac100000u ? 12 : 16;
  if (prefix < privatePrefix)
    return false;
  const auto hostMask = 0xffffffffu >> prefix;
  return (n & hostMask) != 0 && (n & hostMask) != hostMask;
}
bool DiscoveryScope::eligibleLink(
    const QString &name, const QString &label, QNetworkInterface::InterfaceType type,
    QNetworkInterface::InterfaceFlags flags
)
{
  if (type != QNetworkInterface::Ethernet && type != QNetworkInterface::Wifi)
    return false;
  if (!flags.testFlag(QNetworkInterface::IsUp) || !flags.testFlag(QNetworkInterface::IsRunning) ||
      !flags.testFlag(QNetworkInterface::CanMulticast) || flags.testFlag(QNetworkInterface::IsLoopBack) ||
      flags.testFlag(QNetworkInterface::IsPointToPoint))
    return false;
  static const QRegularExpression prefixes(
      QStringLiteral("^(utun|tun|tap|ppp|ipsec|wg|awdl|llw|bridge|vmnet|vbox|virbr|veth|docker|zt|tailscale)"),
      QRegularExpression::CaseInsensitiveOption
  );
  static const QRegularExpression virtualLabels(
      QStringLiteral("vpn|tunnel|virtual|wireguard|tailscale|zerotier|todesk|nolock|hyper-v|loopback"),
      QRegularExpression::CaseInsensitiveOption
  );
  return !prefixes.match(name).hasMatch() && !virtualLabels.match(name + " " + label).hasMatch();
}
QVector<DiscoveryScope> DiscoveryScope::current()
{
  QVector<DiscoveryScope> result;
  for (const auto &network : QNetworkInterface::allInterfaces()) {
    if (!eligibleLink(network.name(), network.humanReadableName(), network.type(), network.flags()) ||
        !platformPhysical(network))
      continue;
    for (const auto &entry : network.addressEntries()) {
      DiscoveryScope scope{static_cast<quint32>(network.index()), network.name(), entry.ip(), entry.prefixLength()};
      if (scope.valid() && !result.contains(scope))
        result.push_back(scope);
    }
  }
  return result;
}
bool DiscoveryAdvertisement::valid() const
{
  static const QRegularExpression format(QStringLiteral("^[0-9a-f]{24}$"));
  return scope.valid() && port > 0 && port <= 65535 && format.match(token).hasMatch();
}
QString DiscoveryAdvertisement::newToken()
{
  return QUuid::createUuid().toString(QUuid::Id128).left(24);
}
} // namespace deskflow::gui
