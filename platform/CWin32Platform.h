#ifndef CWIN32PLATFORM_H
#define CWIN32PLATFORM_H

#include "IPlatform.h"

class CWin32Platform : public IPlatform {
public:
	CWin32Platform();
	virtual ~CWin32Platform();

	// IPlatform overrides
	virtual bool		installDaemon(/* FIXME */);
	virtual bool		uninstallDaemon(/* FIXME */);
	virtual bool		daemonize(const char* name);
	virtual const char*	getBasename(const char* pathname) const;
	virtual CString		getUserDirectory() const;
	virtual CString		getSystemDirectory() const;
	virtual CString		addPathComponent(
								const CString& prefix,
								const CString& suffix) const;

private:
	static void			serviceLogger(int, const char*);
};

#endif
