/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
