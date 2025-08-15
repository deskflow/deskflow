/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/IInterface.h"

#include <functional>

//! Interface for architecture dependent daemonizing
/*!
This interface defines the operations required by deskflow for installing
uninstalling daeamons and daemonizing a process.  Each architecture must
implement this interface.
*/
class IArchDaemon : public IInterface
{
public:
  using DaemonFunc = std::function<int(int, const char **)>;

  //! @name manipulators
  //@{

  //! Install daemon
  /*!
  Install a daemon.  \c name is the name of the daemon passed to the
  system and \c description is a short human readable description of
  the daemon.  \c pathname is the path to the daemon executable.
  \c commandLine should \b not include the name of program as the
  first argument.  If \c allUsers is true then the daemon will be
  installed to start at boot time, otherwise it will be installed to
  start when the current user logs in.  If \p dependencies is not nullptr
  then it's a concatenation of NUL terminated other daemon names
  followed by a NUL;  the daemon will be configured to startup after
  the listed daemons.  Throws an \c ArchDaemonException exception on failure.
  */
  virtual void installDaemon(
      const QString &name, const QString &description, const QString &pathname, const QString &commandLine,
      const QString &dependencies
  ) = 0;

  //! Uninstall daemon
  /*!
  Uninstall a daemon.  Throws an \c ArchDaemonException on failure.
  */
  virtual void uninstallDaemon(const QString &name) = 0;

  //! Install daemon
  /*!
  Installs the default daemon.
  */
  virtual void installDaemon() = 0;

  //! Uninstall daemon
  /*!
  Uninstalls the default daemon.
  */
  virtual void uninstallDaemon() = 0;

  //! Daemonize the process
  /*!
  Daemonize.  Throw ArchDaemonFailedException on error.  \c name is the name
  of the daemon.  Once daemonized, \c func is invoked and daemonize
  returns when and what it does.

  Exactly what happens when daemonizing depends on the platform.
  <ul>
  <li>unix:
    Detaches from terminal.  \c func gets passed one argument, the
    name passed to daemonize().
  <li>win32:
    Becomes a service.  Argument 0 is the name of the service
    and the rest are the arguments passed to StartService().
    \c func is only called when the service is actually started.
    \c func must call \c ArchMiscWindows::runDaemon() to finally
    becoming a service.  The \c runFunc function passed to \c runDaemon()
    must call \c ArchMiscWindows::daemonRunning(true) when it
    enters the main loop (i.e. after initialization) and
    \c ArchMiscWindows::daemonRunning(false) when it leaves
    the main loop.  The \c stopFunc function passed to \c runDaemon()
    is called when the daemon must exit the main loop and it must cause
    \c runFunc to return.  \c func should return what \c runDaemon()
    returns.  \c func or \c runFunc can call
    \c ArchMiscWindows::daemonFailed() to indicate startup failure.
  </ul>
  */
  virtual int daemonize(const QString &name, DaemonFunc const &func) = 0;

  //! Check if user has permission to install the daemon
  /*!
  Returns true iff the caller has permission to install or
  uninstall the daemon.  Note that even if this method returns
  true it's possible that installing/uninstalling the service
  may still fail.  This method ignores whether or not the
  service is already installed.
  */
  virtual bool canInstallDaemon(const QString &name) = 0;

  //! Check if the daemon is installed
  /*!
  Returns true iff the daemon is installed.
  */
  virtual bool isDaemonInstalled(const QString &name) = 0;

  //@}

  //! Get the command line
  /*!
  Gets the command line with which the application was started.
  */
  virtual QString commandLine() const = 0;

  //@}
};
