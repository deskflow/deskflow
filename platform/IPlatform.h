#ifndef IPLATFORM_H
#define IPLATFORM_H

#include "IInterface.h"
#include "CString.h"

class IPlatform : public IInterface {
public:
	typedef int			(*DaemonFunc)(IPlatform*, int argc, const char** argv);

	enum EResult {
		kSuccess,
		kFailed,
		kAlready
	};

	// manipulators

	// install/uninstall a daemon.  commandLine should *not*
	// include the name of program as the first argument.
	// FIXME -- throw on error?  will get better error messages that way.
	virtual bool		installDaemon(const char* name,
							const char* description,
							const char* pathname,
							const char* commandLine) = 0;
	virtual EResult		uninstallDaemon(const char* name) = 0;

	// daemonize.  this should call installDaemonLogger().  returns
	// true iff successful.  the name is the name of the daemon.

	// daemonize.  this should have the side effect of sending log
	// messages to a system message logger since messages can no
	// longer go to the console.  name is the name of the daemon.
	// once daemonized, func is invoked and daemonize returns when
	// and what func does.  daemonize() returns -1 on error.
	//
	// exactly what happens when daemonizing depends on the platform.
	// unix:
	//   detaches from terminal.  func gets one argument, the name
	//   passed to daemonize().
	// win32:
	//   becomes a service.  argument 0 is the name of the service
	//   and the rest are the arguments passed to StartService().
	//   func is only called when the service is actually started.
	//   func must behave like a proper ServiceMain() function;  in
	//   particular, it must call RegisterServiceCtrlHandler() and
	//   SetServiceStatus().
	virtual int			daemonize(const char* name, DaemonFunc func) = 0;

	// directs CLog to send messages to the daemon log.  used when
	// messages should no longer go to the console.  name is used
	// in the log to identify this process.
	virtual void		installDaemonLogger(const char* name) = 0;

	// accessors

	// find the basename in the given pathname
	virtual const char*	getBasename(const char* pathname) const = 0;

	// get the user's home directory.  returns the empty string if
	// this cannot be determined.
	virtual CString		getUserDirectory() const = 0;

	// get the system configuration file directory
	virtual CString		getSystemDirectory() const = 0;

	// concatenate pathname components with a directory separator
	// between them.  this should not check if the resulting path
	// is longer than allowed by the system.  we'll rely on the
	// system calls to tell us that.
	virtual CString		addPathComponent(
							const CString& prefix,
							const CString& suffix) const = 0;
};

#endif
