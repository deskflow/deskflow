/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "common/IInterface.h"
#include "base/String.h"

//! Interface for architecture dependent daemonizing
/*!
This interface defines the operations required by synergy for installing
uninstalling daeamons and daemonizing a process.  Each architecture must
implement this interface.
*/
class IArchDaemon : public IInterface {
public:
    typedef int (*DaemonFunc) (int argc, const char** argv);

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
    start when the current user logs in.  If \p dependencies is not NULL
    then it's a concatenation of NUL terminated other daemon names
    followed by a NUL;  the daemon will be configured to startup after
    the listed daemons.  Throws an \c XArchDaemon exception on failure.
    */
    virtual void installDaemon (const char* name, const char* description,
                                const char* pathname, const char* commandLine,
                                const char* dependencies) = 0;

    //! Uninstall daemon
    /*!
    Uninstall a daemon.  Throws an \c XArchDaemon on failure.
    */
    virtual void uninstallDaemon (const char* name) = 0;

    //! Install daemon
    /*!
    Installs the default daemon.
    */
    virtual void installDaemon () = 0;

    //! Uninstall daemon
    /*!
    Uninstalls the default daemon.
    */
    virtual void uninstallDaemon () = 0;

    //! Daemonize the process
    /*!
    Daemonize.  Throw XArchDaemonFailed on error.  \c name is the name
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
    virtual int daemonize (const char* name, DaemonFunc func) = 0;

    //! Check if user has permission to install the daemon
    /*!
    Returns true iff the caller has permission to install or
    uninstall the daemon.  Note that even if this method returns
    true it's possible that installing/uninstalling the service
    may still fail.  This method ignores whether or not the
    service is already installed.
    */
    virtual bool canInstallDaemon (const char* name) = 0;

    //! Check if the daemon is installed
    /*!
    Returns true iff the daemon is installed.
    */
    virtual bool isDaemonInstalled (const char* name) = 0;

    //@}

    //! Get the command line
    /*!
    Gets the command line with which the application was started.
    */
    virtual std::string commandLine () const = 0;

    //@}
};
