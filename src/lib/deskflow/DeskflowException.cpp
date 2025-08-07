/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/DeskflowException.h"
#include "base/String.h"

//
// BadClientException
//

std::string BadClientException::getWhat() const throw()
{
  return "BadClientException";
}

//
// InvalidProtocolException
//

std::string InvalidProtocolException::getWhat() const throw()
{
  return "InvalidProtocolException; Check the options section of the server "
         "configuration file has a valid protocol defined.\nProtocol must be barrier (default) or synergy."
         "\n Ex:\nsection: options\n\tprotocol = barrier\nend\n";
}

//
// IncompatibleClientException
//

IncompatibleClientException::IncompatibleClientException(int major, int minor) : m_major(major), m_minor(minor)
{
  // do nothing
}

int IncompatibleClientException::getMajor() const noexcept
{
  return m_major;
}

int IncompatibleClientException::getMinor() const noexcept
{
  return m_minor;
}

std::string IncompatibleClientException::getWhat() const throw()
{
  return format(
      "IncompatibleClientException", "incompatible client %{1}.%{2}", deskflow::string::sprintf("%d", m_major).c_str(),
      deskflow::string::sprintf("%d", m_minor).c_str()
  );
}

//
// DuplicateClientException
//

DuplicateClientException::DuplicateClientException(const std::string &name) : m_name(name)
{
  // do nothing
}

const std::string &DuplicateClientException::getName() const noexcept
{
  return m_name;
}

std::string DuplicateClientException::getWhat() const throw()
{
  return format("DuplicateClientException", "duplicate client %{1}", m_name.c_str());
}

//
// UnknownClientException
//

UnknownClientException::UnknownClientException(const std::string &name) : m_name(name)
{
  // do nothing
}

const std::string &UnknownClientException::getName() const noexcept
{
  return m_name;
}

std::string UnknownClientException::getWhat() const throw()
{
  return format("UnknownClientException", "unknown client %{1}", m_name.c_str());
}

//
// ExitAppException
//

ExitAppException::ExitAppException(int code) : m_code(code)
{
  // do nothing
}

int ExitAppException::getCode() const noexcept
{
  return m_code;
}

std::string ExitAppException::getWhat() const throw()
{
  return format("ExitAppException", "exiting with code %{1}", deskflow::string::sprintf("%d", m_code).c_str());
}
