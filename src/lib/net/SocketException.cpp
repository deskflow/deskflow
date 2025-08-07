/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/SocketException.h"
#include "base/String.h"

//
// SocketAddressException
//

SocketAddressException::SocketAddressException(SocketError error, const std::string &hostname, int port) noexcept
    : m_error(error),
      m_hostname(hostname),
      m_port(port)
{
  // do nothing
}

SocketAddressException::SocketError SocketAddressException::getError() const noexcept
{
  return m_error;
}

std::string SocketAddressException::getHostname() const noexcept
{
  return m_hostname;
}

int SocketAddressException::getPort() const noexcept
{
  return m_port;
}

std::string SocketAddressException::getWhat() const throw()
{
  static const char *s_errorID[] = {
      "SocketAddressUnknownException", "SocketAddressNotFoundException", "SocketAddressNoAddressException",
      "SocketAddressUnsupportedException", "SocketAddressBadPortException"
  };
  static const char *s_errorMsg[] = {
      "unknown error for: %{1}:%{2}", "address not found for: %{1}", "no address for: %{1}",
      "unsupported address for: %{1}",
      "invalid port" // m_port may not be set to the bad port
  };
  const auto index = static_cast<int>(m_error);
  return format(
      s_errorID[index], s_errorMsg[index], m_hostname.c_str(), deskflow::string::sprintf("%d", m_port).c_str()
  );
}

//
// SocketIOCloseException
//

std::string SocketIOCloseException::getWhat() const throw()
{
  return format("SocketIOCloseException", "close: %{1}", what());
}

//
// SocketBindException
//

std::string SocketBindException::getWhat() const throw()
{
  return format("SocketBindException", "cannot bind address: %{1}", what());
}

//
// SocketAddressInUseException
//

std::string SocketAddressInUseException::getWhat() const throw()
{
  return format("SocketAddressInUseException", "cannot bind address: %{1}", what());
}

//
// SocketConnectException
//

std::string SocketConnectException::getWhat() const throw()
{
  return format("SocketConnectException", "cannot connect socket: %{1}", what());
}

//
// SocketCreateException
//

std::string SocketCreateException::getWhat() const throw()
{
  return format("SocketCreateException", "cannot create socket: %{1}", what());
}
