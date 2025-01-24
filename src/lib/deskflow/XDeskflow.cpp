/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/XDeskflow.h"
#include "base/String.h"

//
// XBadClient
//

std::string XBadClient::getWhat() const throw()
{
  return "XBadClient";
}

//
// XInvalidProtocol
//

std::string XInvalidProtocol::getWhat() const throw()
{
  return "XInvalidProtocol";
}

//
// XIncompatibleClient
//

XIncompatibleClient::XIncompatibleClient(int major, int minor) : m_major(major), m_minor(minor)
{
  // do nothing
}

int XIncompatibleClient::getMajor() const throw()
{
  return m_major;
}

int XIncompatibleClient::getMinor() const throw()
{
  return m_minor;
}

std::string XIncompatibleClient::getWhat() const throw()
{
  return format(
      "XIncompatibleClient", "incompatible client %{1}.%{2}", deskflow::string::sprintf("%d", m_major).c_str(),
      deskflow::string::sprintf("%d", m_minor).c_str()
  );
}

//
// XDuplicateClient
//

XDuplicateClient::XDuplicateClient(const std::string &name) : m_name(name)
{
  // do nothing
}

const std::string &XDuplicateClient::getName() const throw()
{
  return m_name;
}

std::string XDuplicateClient::getWhat() const throw()
{
  return format("XDuplicateClient", "duplicate client %{1}", m_name.c_str());
}

//
// XUnknownClient
//

XUnknownClient::XUnknownClient(const std::string &name) : m_name(name)
{
  // do nothing
}

const std::string &XUnknownClient::getName() const throw()
{
  return m_name;
}

std::string XUnknownClient::getWhat() const throw()
{
  return format("XUnknownClient", "unknown client %{1}", m_name.c_str());
}

//
// XExitApp
//

XExitApp::XExitApp(int code) : m_code(code)
{
  // do nothing
}

int XExitApp::getCode() const throw()
{
  return m_code;
}

std::string XExitApp::getWhat() const throw()
{
  return format("XExitApp", "exiting with code %{1}", deskflow::string::sprintf("%d", m_code).c_str());
}
