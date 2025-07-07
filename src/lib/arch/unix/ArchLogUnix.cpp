/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/unix/ArchLogUnix.h"

#include <syslog.h>

//
// ArchLogUnix
//

void ArchLogUnix::openLog(const char *name)
{
  openlog(name, 0, LOG_DAEMON);
}

void ArchLogUnix::closeLog()
{
  closelog();
}

void ArchLogUnix::showLog(bool)
{
  // do nothing
}

void ArchLogUnix::writeLog(LogLevel level, const char *msg)
{
  // convert level
  int priority;
  switch (level) {
    using enum LogLevel;
  case Error:
    priority = LOG_ERR;
    break;

  case Warning:
    priority = LOG_WARNING;
    break;

  case Note:
    priority = LOG_NOTICE;
    break;

  case Info:
    priority = LOG_INFO;
    break;

  default:
    priority = LOG_DEBUG;
    break;
  }

  // log it
  syslog(priority, "%s", msg);
}
