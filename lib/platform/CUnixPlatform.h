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

#ifndef CUNIXPLATFORM_H
#define CUNIXPLATFORM_H

#include "IPlatform.h"

//! Unix platform dependent functions
class CUnixPlatform : public IPlatform {
public:
	CUnixPlatform();
	virtual ~CUnixPlatform();

	// IPlatform overrides
	virtual bool		installDaemon(const char* name,
							const char* description,
							const char* pathname,
							const char* commandLine,
							bool allUsers);
	virtual EResult		uninstallDaemon(const char* name, bool allUsers);
	virtual int			daemonize(const char* name, DaemonFunc);
	virtual void		installDaemonLogger(const char* name);
	virtual bool		canInstallDaemon(const char* name,
							bool allUsers) const;
	virtual bool		isDaemonInstalled(const char* name,
							bool allUsers) const;
	virtual const char*	getBasename(const char* pathname) const;
	virtual CString		getUserDirectory() const;
	virtual CString		getSystemDirectory() const;
	virtual CString		addPathComponent(
							const CString& prefix,
							const CString& suffix) const;

private:
	static bool			deamonLogger(int, const char*);
};

#endif
