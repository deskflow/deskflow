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

ArchLogUnix::ArchLogUnix()
{
  // do nothing
}

ArchLogUnix::~ArchLogUnix()
{
  // do nothing
}

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

void ArchLogUnix::writeLog(ELevel level, const char *msg)
{
  // convert level
  int priority;
  switch (level) {
  case kERROR:
    priority = LOG_ERR;
    break;

  case kWARNING:
    priority = LOG_WARNING;
    break;

  case kNOTE:
    priority = LOG_NOTICE;
    break;

  case kINFO:
    priority = LOG_INFO;
    break;

  default:
    priority = LOG_DEBUG;
    break;
  }

  // log it
  syslog(priority, "%s", msg);
}
