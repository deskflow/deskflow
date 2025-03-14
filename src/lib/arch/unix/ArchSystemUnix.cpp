/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/unix/ArchSystemUnix.h"
#include <array>

#include <common/constants.h>
#include <sys/utsname.h>

#ifndef __APPLE__
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusReply>
#endif

//
// ArchSystemUnix
//

ArchSystemUnix::ArchSystemUnix()
{
  // do nothing
}

ArchSystemUnix::~ArchSystemUnix()
{
  // do nothing
}

std::string ArchSystemUnix::setting(const std::string &) const
{
  return "";
}

void ArchSystemUnix::setting(const std::string &, const std::string &) const
{
}

void ArchSystemUnix::clearSettings() const
{
  // Not implemented
}

std::string ArchSystemUnix::getLibsUsed(void) const
{
  return "not implemented.\nuse lsof on shell";
}
