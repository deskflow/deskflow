/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/ArchDaemonNone.h"
#include <QString>

//
// ArchDaemonNone
//

void ArchDaemonNone::installDaemon(const QString &, const QString &, const QString &, const QString &, const QString &)
{
  // do nothing
}

void ArchDaemonNone::uninstallDaemon(const QString &)
{
  // do nothing
}

int ArchDaemonNone::daemonize(const QString &name, DaemonFunc const &func)
{
  // simply forward the call to func.  obviously, this doesn't
  // do any daemonizing.
  auto t = name.toStdString();
  const char *n = t.c_str();
  return func(1, &n);
}

bool ArchDaemonNone::canInstallDaemon(const QString &)
{
  return false;
}

bool ArchDaemonNone::isDaemonInstalled(const QString &)
{
  return false;
}

void ArchDaemonNone::installDaemon()
{
  // do nothing
}

void ArchDaemonNone::uninstallDaemon()
{
  // do nothing
}

QString ArchDaemonNone::commandLine() const
{
  return {};
}
