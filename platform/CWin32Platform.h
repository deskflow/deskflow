#ifndef CWIN32PLATFORM_H
#define CWIN32PLATFORM_H

#include "IPlatform.h"
#include "CCondVar.h"
#include "CMutex.h"
#include <windows.h>

class CWin32Platform : public IPlatform {
public:
	typedef int			(*RunFunc)(CMutex*);
	typedef void		(*StopFunc)(void);

	CWin32Platform();
	virtual ~CWin32Platform();

	// returns true iff the platform is win95/98/me
	static bool			isWindows95Family();

	// utility for calling SetServiceStatus()
	static void			setStatus(SERVICE_STATUS_HANDLE, DWORD state);
	static void			setStatus(SERVICE_STATUS_HANDLE,
								DWORD state, DWORD step, DWORD waitHint);
	static void			setStatusError(SERVICE_STATUS_HANDLE, DWORD error);

	// run a service.  the RunFunc should unlock the passed in mutex
	// (which will be locked on entry) when not initializing or
	// shutting down (i.e. when running its loop).  StopFunc should
	// cause the RunFunc() to return.  returns what RunFunc returns.
	// RunFunc should throw CDaemonFailed if the service fails.
	int					runDaemon(RunFunc, StopFunc);

	// thrown by RunFunc on service failure.  result is the error
	// code reported by the service.
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

	void				serviceMain(DWORD, LPTSTR*);
	static void WINAPI	serviceMainEntry(DWORD, LPTSTR*);

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
