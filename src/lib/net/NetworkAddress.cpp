/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/NetworkAddress.h"

#include "arch/Arch.h"
#include "arch/XArch.h"
#include "net/XSocket.h"

#include <algorithm>
#include <cstdlib>

//
// NetworkAddress
//

// name re-resolution adapted from a patch by Brent Priddy.

NetworkAddress::NetworkAddress(int port) : m_port(port)
{
  checkPort();
  m_address = ARCH->newAnyAddr(IArchNetwork::kINET);
  ARCH->setAddrPort(m_address, m_port);
}

NetworkAddress::NetworkAddress(const NetworkAddress &addr) : m_hostname(addr.m_hostname), m_port(addr.m_port)
{
  *this = addr;
}

NetworkAddress::NetworkAddress(const std::string &hostname, int port) : m_hostname(hostname), m_port(port)
{
  // detect internet protocol version with colom count
  auto isColomPredicate = [](char c) { return c == ':'; };
  auto colomCount = std::count_if(m_hostname.begin(), m_hostname.end(), isColomPredicate);

  if (colomCount == 1) {
    // ipv4 with port part
    auto hostIt = m_hostname.find(':');
    try {
      m_port = std::stoi(m_hostname.substr(hostIt + 1));
    } catch (...) {
      throw XSocketAddress(XSocketAddress::kBadPort, m_hostname, m_port);
    }

    auto endHostnameIt = static_cast<int>(hostIt);
    m_hostname = m_hostname.substr(0, endHostnameIt > 0 ? endHostnameIt : 0);
  } else if (colomCount > 1) {
    // ipv6 part
    if (m_hostname[0] == '[') {
      // ipv6 with port part
      std::string portDelimeter = "]:";
      auto hostIt = m_hostname.find(portDelimeter);

      // bad syntax of ipv6 with port
      if (hostIt == std::string::npos) {
        throw XSocketAddress(XSocketAddress::kUnknown, m_hostname, m_port);
      }

      auto portSuffix = m_hostname.substr(hostIt + portDelimeter.size());
      // port is implied but omitted
      if (portSuffix.empty()) {
        throw XSocketAddress(XSocketAddress::kBadPort, m_hostname, m_port);
      }
      try {
        m_port = std::stoi(portSuffix);
      } catch (...) {
        // port is not a number
        throw XSocketAddress(XSocketAddress::kBadPort, m_hostname, m_port);
      }

      auto endHostnameIt = static_cast<int>(hostIt) - 1;
      m_hostname = m_hostname.substr(1, endHostnameIt > 0 ? endHostnameIt : 0);
    }

    // ensure that ipv6 link-local adress ended with scope id
    if (m_hostname.rfind("fe80:", 0) == 0 && m_hostname.find('%') == std::string::npos) {
      throw XSocketAddress(XSocketAddress::kUnknown, m_hostname, m_port);
    }
  }

  // check port number
  checkPort();
}

NetworkAddress::~NetworkAddress()
{
  if (m_address != nullptr) {
    ARCH->closeAddr(m_address);
    m_address = nullptr;
  }
}

NetworkAddress &NetworkAddress::operator=(const NetworkAddress &addr)
{
  if (m_address != nullptr) {
    ARCH->closeAddr(m_address);
    m_address = nullptr;
  }

  ArchNetAddress newAddr = nullptr;
  if (addr.m_address != nullptr) {
    newAddr = ARCH->copyAddr(addr.m_address);
  }
  m_address = newAddr;

  m_hostname = addr.m_hostname;
  m_port = addr.m_port;
  return *this;
}

size_t NetworkAddress::resolve(size_t index)
{
  size_t resolvedAddressesCount = 0;
  // discard previous address
  if (m_address != nullptr) {
    ARCH->closeAddr(m_address);
    m_address = nullptr;
  }

  try {
    // if hostname is empty then use wildcard address otherwise look
    // up the name.
    if (m_hostname.empty()) {
      m_address = ARCH->newAnyAddr(IArchNetwork::kINET);
      resolvedAddressesCount = 1;
    } else {
      // Logic for temporary filtring only ipv4 addresses
      std::vector<ArchNetAddress> ipv4OnlyAddresses;
      {
        auto addresses = ARCH->nameToAddr(m_hostname);
        for (auto address : addresses) {
          if (ARCH->getAddrFamily(address) == IArchNetwork::kINET) {
            ipv4OnlyAddresses.emplace_back(address);
          } else {
            ARCH->closeAddr(address);
          }
        }
      }

      resolvedAddressesCount = ipv4OnlyAddresses.size();
      assert(resolvedAddressesCount > 0);
      if (index < resolvedAddressesCount - 1) {
        m_address = ipv4OnlyAddresses[index];
      } else {
        m_address = ipv4OnlyAddresses[resolvedAddressesCount - 1];
      }

      for (auto address : ipv4OnlyAddresses) {
        if (m_address != address) {
          ARCH->closeAddr(address);
        }
      }
    }
  } catch (XArchNetworkNameUnknown &) {
    throw XSocketAddress(XSocketAddress::kNotFound, m_hostname, m_port);
  } catch (XArchNetworkNameNoAddress &) {
    throw XSocketAddress(XSocketAddress::kNoAddress, m_hostname, m_port);
  } catch (XArchNetworkNameUnsupported &) {
    throw XSocketAddress(XSocketAddress::kUnsupported, m_hostname, m_port);
  } catch (XArchNetworkName &) {
    throw XSocketAddress(XSocketAddress::kUnknown, m_hostname, m_port);
  }

  // set port in address
  ARCH->setAddrPort(m_address, m_port);

  return resolvedAddressesCount;
}

bool NetworkAddress::operator==(const NetworkAddress &addr) const
{
  return m_address == addr.m_address || ARCH->isEqualAddr(m_address, addr.m_address);
}

bool NetworkAddress::operator!=(const NetworkAddress &addr) const
{
  return !operator==(addr);
}

bool NetworkAddress::isValid() const
{
  return (m_address != nullptr);
}

const ArchNetAddress &NetworkAddress::getAddress() const
{
  return m_address;
}

int NetworkAddress::getPort() const
{
  return m_port;
}

std::string NetworkAddress::getHostname() const
{
  return m_hostname;
}

void NetworkAddress::checkPort()
{
  // check port number
  if (m_port < 0 || m_port > 65535) {
    throw XSocketAddress(XSocketAddress::kBadPort, m_hostname, m_port);
  }
}
