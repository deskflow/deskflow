/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/XBase.h"
#include "base/String.h"

#include <cerrno>
#include <cstdarg>
#include <cstring>

//
// XBase
//

XBase::XBase() : std::runtime_error("")
{
  // do nothing
}

XBase::XBase(const std::string &msg) : std::runtime_error(msg)
{
  // do nothing
}

XBase::~XBase() _NOEXCEPT
{
  // do nothing
}

const char *XBase::what() const _NOEXCEPT
{
  if (const char *what = std::runtime_error::what(); what != nullptr && what[0] != '\0') {
    return what;
  }

  m_what = getWhat();
  return m_what.c_str();
}

std::string XBase::format(const char * /*id*/, const char *fmt, ...) const throw()
{
  // FIXME -- lookup message string using id as an index.  set
  // fmt to that string if it exists.

  // format
  std::string result;
  va_list args;
  va_start(args, fmt);
  try {
    result = deskflow::string::vformat(fmt, args);
  } catch (...) {
    // ignore
    result.clear();
  }
  va_end(args);

  return result;
}
