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

#ifndef CWIN32PLATFORM_H
#define CWIN32PLATFORM_H

#include "IPlatform.h"
#include "CCondVar.h"
#include "CMutex.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//! Microsoft windows platform dependent functions
class CWin32Platform : public IPlatform {
public:
	typedef int			(*RunFunc)(CMutex*);
	typedef void		(*StopFunc)(void);

	CWin32Platform();
	virtual ~CWin32Platform();

	//! Test if windows 95, et al.
	/*!
	Returns true iff the platform is win95/98/me.
	*/
	static bool			isWindows95Family();

	//! Utility for calling SetServiceStatus()
	static void			setStatus(SERVICE_STATUS_HANDLE, DWORD state);
	//! Utility for calling SetServiceStatus()
	static void			setStatus(SERVICE_STATUS_HANDLE,
							DWORD state, DWORD step, DWORD waitHint);
	//! Utility for calling SetServiceStatus()
	static void			setStatusError(SERVICE_STATUS_HANDLE, DWORD error);

	//! Run service
	/*!
	Run a service.  The RunFunc should unlock the passed in mutex
	(which will be locked on entry) when not initializing or
	shutting down (i.e. when running its loop).  StopFunc should
	cause the RunFunc() to return.  Returns what RunFunc returns.
	RunFunc should throw CDaemonFailed if the service fails.
	*/
	int					runDaemon(RunFunc, StopFunc);

	//! Daemon failed exception
	/*!
	Thrown by RunFunc on service failure.  Result is the error
	code reported by the service.
	*/
	class CDaemonFailed {
	public:
		CDaemonFailed(int result) : m_result(result) { }

	public:
		int				m_result;
	};

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
	static HKEY			openKey(HKEY parent, const char*);
	static HKEY			openKey(HKEY parent, const char**);
	static void			closeKey(HKEY);
	static void			deleteKey(HKEY, const char* name);
	static void			deleteValue(HKEY, const char* name);
	static void			setValue(HKEY, const char* name,
							const CString& value);
	static CString		readValueString(HKEY, const char* name);
	static HKEY			openNTServicesKey();
	static HKEY			open95ServicesKey();
	static HKEY			openUserStartupKey();

	void				serviceMain(DWORD, LPTSTR*);
	static void WINAPI	serviceMainEntry(DWORD, LPTSTR*);

	void				runDaemonThread(void*);

	void				serviceHandler(DWORD ctrl);
	static void WINAPI	serviceHandlerEntry(DWORD ctrl);

	static bool			serviceLogger(int, const char*);

private:
	DaemonFunc			m_daemonFunc;
	int					m_daemonResult;

	SERVICE_STATUS_HANDLE m_statusHandle;

	CMutex*				m_serviceMutex;
	CCondVar<DWORD>*	m_serviceState;
	bool				m_serviceHandlerWaiting;
	bool				m_serviceRunning;
	StopFunc			m_stop;

	static HANDLE		s_eventLog;

	static CWin32Platform*	s_daemonPlatform;
};

#endif
