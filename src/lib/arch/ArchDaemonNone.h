/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchDaemon.h"

#define ARCH_DAEMON ArchDaemonNone

//! Dummy implementation of IArchDaemon
/*!
This class implements IArchDaemon for a platform that does not have
daemons.  The install and uninstall functions do nothing, the query
functions return false, and \c daemonize() simply calls the passed
function and returns its result.
*/
class ArchDaemonNone : public IArchDaemon
{
public:
  ArchDaemonNone();
  virtual ~ArchDaemonNone();

  // IArchDaemon overrides
  virtual void installDaemon(
      const char *name, const char *description, const char *pathname, const char *commandLine, const char *dependencies
  );
  virtual void uninstallDaemon(const char *name);
  virtual int daemonize(const char *name, DaemonFunc func);
  virtual bool canInstallDaemon(const char *name);
  virtual bool isDaemonInstalled(const char *name);
  virtual void installDaemon();
  virtual void uninstallDaemon();
  virtual std::string commandLine() const;
};
