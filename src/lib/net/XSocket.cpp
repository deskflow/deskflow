/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/XSocket.h"
#include "base/String.h"

//
// XSocketAddress
//

XSocketAddress::XSocketAddress(EError error, const std::string &hostname, int port) _NOEXCEPT : m_error(error),
                                                                                                m_hostname(hostname),
                                                                                                m_port(port)
{
  // do nothing
}

XSocketAddress::EError XSocketAddress::getError() const throw()
{
  return m_error;
}

std::string XSocketAddress::getHostname() const throw()
{
  return m_hostname;
}

int XSocketAddress::getPort() const throw()
{
  return m_port;
}

std::string XSocketAddress::getWhat() const throw()
{
  static const char *s_errorID[] = {
      "XSocketAddressUnknown", "XSocketAddressNotFound", "XSocketAddressNoAddress", "XSocketAddressUnsupported",
      "XSocketAddressBadPort"
  };
  static const char *s_errorMsg[] = {
      "unknown error for: %{1}:%{2}", "address not found for: %{1}", "no address for: %{1}",
      "unsupported address for: %{1}",
      "invalid port" // m_port may not be set to the bad port
  };
  return format(
      s_errorID[m_error], s_errorMsg[m_error], m_hostname.c_str(), deskflow::string::sprintf("%d", m_port).c_str()
  );
}

//
// XSocketIOClose
//

std::string XSocketIOClose::getWhat() const throw()
{
  return format("XSocketIOClose", "close: %{1}", what());
}

//
// XSocketBind
//

std::string XSocketBind::getWhat() const throw()
{
  return format("XSocketBind", "cannot bind address: %{1}", what());
}

//
// XSocketConnect
//

std::string XSocketConnect::getWhat() const throw()
{
  return format("XSocketConnect", "cannot connect socket: %{1}", what());
}

//
// XSocketCreate
//

std::string XSocketCreate::getWhat() const throw()
{
  return format("XSocketCreate", "cannot create socket: %{1}", what());
}
