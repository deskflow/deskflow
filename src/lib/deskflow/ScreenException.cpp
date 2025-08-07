/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ScreenException.h"

//
// ScreenOpenFailureException
//

std::string ScreenOpenFailureException::getWhat() const throw()
{
  return format("ScreenOpenFailureException", "unable to open screen");
}

//
// ScreenUnavailableException
//

ScreenUnavailableException::ScreenUnavailableException(double timeUntilRetry) : m_timeUntilRetry(timeUntilRetry)
{
  // do nothing
}

double ScreenUnavailableException::getRetryTime() const
{
  return m_timeUntilRetry;
}

std::string ScreenUnavailableException::getWhat() const throw()
{
  return format("ScreenUnavailableException", "unable to open screen");
}
