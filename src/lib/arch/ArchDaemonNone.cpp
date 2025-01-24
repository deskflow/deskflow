/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/ArchDaemonNone.h"

//
// ArchDaemonNone
//

ArchDaemonNone::ArchDaemonNone()
{
  // do nothing
}

ArchDaemonNone::~ArchDaemonNone()
{
  // do nothing
}

void ArchDaemonNone::installDaemon(const char *, const char *, const char *, const char *, const char *)
{
  // do nothing
}

void ArchDaemonNone::uninstallDaemon(const char *)
{
  // do nothing
}

int ArchDaemonNone::daemonize(const char *name, DaemonFunc func)
{
  // simply forward the call to func.  obviously, this doesn't
  // do any daemonizing.
  return func(1, &name);
}

bool ArchDaemonNone::canInstallDaemon(const char *)
{
  return false;
}

bool ArchDaemonNone::isDaemonInstalled(const char *)
{
  return false;
}

void ArchDaemonNone::installDaemon()
{
}

void ArchDaemonNone::uninstallDaemon()
{
}

std::string ArchDaemonNone::commandLine() const
{
  return "";
}
