/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/DisplayException.h"

//
// DisplayOpenFailureException
//

QString DisplayOpenFailureException::getWhat() const throw()
{
  return format("DisplayOpenFailureException", "unable to open display");
}

//
// X11DisplayUnavailableException
//

QString X11DisplayUnavailableException::getWhat() const throw()
{
  return format("X11DisplayUnavailableException", "unable to open x11 display");
}
