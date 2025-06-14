/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/unix/ArchLogUnix.h"

#include <QString>
#include <syslog.h>
//
// ArchLogUnix
//

void ArchLogUnix::openLog(const QString &name)
{
  openlog(name.toStdString().c_str(), 0, LOG_DAEMON);
}

void ArchLogUnix::closeLog()
{
  closelog();
}

void ArchLogUnix::showLog(bool)
{
  // do nothing
}

void ArchLogUnix::writeLog(ELevel level, const QString &msg)
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
  syslog(priority, "%s", msg.toStdString().c_str());
}
