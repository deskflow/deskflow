#ifndef IPLATFORM_H
#define IPLATFORM_H

#include "BasicTypes.h"
#include "CString.h"
#include "IInterface.h"

class IPlatform : public IInterface {
public:
	// manipulators

	// install/uninstall a daemon. 
	// FIXME -- throw on error?  will get better error messages that way.
	virtual bool		installDaemon(/* FIXME */) = 0;
	virtual bool		uninstallDaemon(/* FIXME */) = 0;

	// daemonize.  this should have the side effect of sending log
	// messages to a system message logger since messages can no
	// longer go to the console.  returns true iff successful.
	// the name is the name of the daemon.
// FIXME -- win32 services will require a more complex interface
	virtual bool		daemonize(const char* name) = 0;

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
