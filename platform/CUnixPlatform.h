#ifndef CUNIXPLATFORM_H
#define CUNIXPLATFORM_H

#include "IPlatform.h"

class CUnixPlatform : public IPlatform {
public:
	CUnixPlatform();
	virtual ~CUnixPlatform();

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

protected:
	virtual void		setDaemonLogger(const char* name);

private:
	static void			deamonLogger(int, const char*);
};

#endif
