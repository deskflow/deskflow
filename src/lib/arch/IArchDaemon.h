/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>
#include <functional>

//! Interface for architecture dependent daemonizing
/*!
This interface defines the operations required by deskflow for installing
uninstalling daeamons and daemonizing a process.  Each architecture must
implement this interface.
*/
class IArchDaemon
{
public:
  using DaemonFunc = std::function<int()>;

  virtual ~IArchDaemon() = default;
  //! @name manipulators
  //@{

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
    must call \c ArchDaemonWindows::daemonRunning(true) when it
    enters the main loop (i.e. after initialization) and
    \c ArchDaemonWindows::daemonRunning(false) when it leaves
    the main loop.  The \c stopFunc function passed to \c runDaemon()
    is called when the daemon must exit the main loop and it must cause
    \c runFunc to return.  \c func should return what \c runDaemon()
    returns.  \c func or \c runFunc can call
    \c ArchDaemonWindows::daemonFailed() to indicate startup failure.
  </ul>
  */
  virtual int daemonize(DaemonFunc const &func) = 0;

  //@}

  //! Get the command line
  /*!
  Gets the command line with which the application was started.
  */
  virtual QString commandLine() const = 0;

  //@}
};
