/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/XScreen.h"

//
// XScreenOpenFailure
//

std::string XScreenOpenFailure::getWhat() const throw()
{
  return format("XScreenOpenFailure", "unable to open screen");
}

//
// XScreenXInputFailure
//

std::string XScreenXInputFailure::getWhat() const throw()
{
  return "";
}

//
// XScreenUnavailable
//

XScreenUnavailable::XScreenUnavailable(double timeUntilRetry) : m_timeUntilRetry(timeUntilRetry)
{
  // do nothing
}

XScreenUnavailable::~XScreenUnavailable() _NOEXCEPT
{
  // do nothing
}

double XScreenUnavailable::getRetryTime() const
{
  return m_timeUntilRetry;
}

std::string XScreenUnavailable::getWhat() const throw()
{
  return format("XScreenUnavailable", "unable to open screen");
}
