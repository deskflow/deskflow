#ifndef CUNIXPLATFORM_H
#define CUNIXPLATFORM_H

#include "IPlatform.h"

class CUnixPlatform : public IPlatform {
public:
	CUnixPlatform();
	virtual ~CUnixPlatform();

	// IPlatform overrides
	virtual bool		installDaemon(const char* name,
								const char* description,
								const char* pathname,
								const char* commandLine);
	virtual bool		uninstallDaemon(const char* name);
	virtual int			daemonize(const char* name, DaemonFunc);
	virtual int			restart(RestartFunc, int minErrorCode);
	virtual const char*	getBasename(const char* pathname) const;
	virtual CString		getUserDirectory() const;
	virtual CString		getSystemDirectory() const;
	virtual CString		addPathComponent(
								const CString& prefix,
								const CString& suffix) const;

protected:
	virtual void		setDaemonLogger(const char* name);

private:
	static bool			deamonLogger(int, const char*);
};

#endif
