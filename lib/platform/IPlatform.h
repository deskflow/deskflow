/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef IPLATFORM_H
#define IPLATFORM_H

#include "IInterface.h"
#include "CString.h"

//! Platform dependent functions interface
/*!
This interface provides methods to query platform specific data and
perform operations that are inherently non-portable.
*/
class IPlatform : public IInterface {
public:
	typedef int			(*DaemonFunc)(IPlatform*, int argc, const char** argv);

	//! Uninstall result codes
	enum EResult {
		kSuccess,		//!< Uninstall succeeded
		kFailed,		//!< Uninstall failed
		kAlready		//!< Not installed
	};

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
	start when the current user logs in.
	*/
	// FIXME -- throw on error?  will get better error messages that way.
	virtual bool		installDaemon(const char* name,
							const char* description,
							const char* pathname,
							const char* commandLine,
							bool allUsers) = 0;

	//! Uninstall daemon
	/*!
	Uninstall a daemon.
	*/
	virtual EResult		uninstallDaemon(const char* name, bool allUsers) = 0;

	//! Daemonize the process
	/*!
	Daemonize.  This should call installDaemonLogger().  Returns
	true iff successful.  \c name is the name of the daemon.
	Once daemonized, \c func is invoked and daemonize returns when
	and what it does.  daemonize() returns -1 on error.
	
	Exactly what happens when daemonizing depends on the platform.
	<ul>
	<li>unix:
	  Detaches from terminal.  \c func gets passed one argument, the
	  name passed to daemonize().
	<li>win32:
	  Becomes a service.  Argument 0 is the name of the service
	  and the rest are the arguments passed to StartService().
	  \c func is only called when the service is actually started.
	  \c func must behave like a proper ServiceMain() function;  in
	  particular, it must call RegisterServiceCtrlHandler() and
	  SetServiceStatus().
	</ul>
	*/
	virtual int			daemonize(const char* name, DaemonFunc func) = 0;

	//! Install daemon logger
	/*!
	Directs CLog to send messages to the daemon log.  Used when
	messages should no longer go to the console.  \c name is used
	in the log to identify this process.
	*/
	virtual void		installDaemonLogger(const char* name) = 0;

	//@}
	//! @name accessors
	//@{

	//! Check if user has permission to install the daemon
	/*!
	Returns true iff the caller has permission to install or
	uninstall the daemon.  Note that even if this method returns
	true it's possible that installing/uninstalling the service
	may still fail.  This method ignores whether or not the
	service is already installed.
	*/
	virtual bool		canInstallDaemon(const char* name,
							bool allUsers) const = 0;

	//! Check if the daemon is installed
	/*!
	Returns true iff the daemon is installed.
	*/
	virtual bool		isDaemonInstalled(const char* name,
							bool allUsers) const = 0;

	//! Extract base name
	/*!
	Find the base name in the given \c pathname.
	*/
	virtual const char*	getBasename(const char* pathname) const = 0;

	//! Get user's home directory
	/*!
	Returns the user's home directory.  Returns the empty string if
	this cannot be determined.
	*/
	virtual CString		getUserDirectory() const = 0;

	//! Get system directory
	/*!
	Returns the ussystem configuration file directory.
	*/
	virtual CString		getSystemDirectory() const = 0;

	//! Concatenate path components
	/*!
	Concatenate pathname components with a directory separator
	between them.  This should not check if the resulting path
	is longer than allowed by the system;  we'll rely on the
	system calls to tell us that.
	*/
	virtual CString		addPathComponent(
							const CString& prefix,
							const CString& suffix) const = 0;

	//@}
};

#endif
