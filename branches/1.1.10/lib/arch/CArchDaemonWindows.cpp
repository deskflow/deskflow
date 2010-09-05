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

#include "CArchDaemonWindows.h"
#include "CArch.h"
#include "CArchMiscWindows.h"
#include "XArchWindows.h"
#include "stdvector.h"

//
// CArchDaemonWindows
//

CArchDaemonWindows*		CArchDaemonWindows::s_daemon = NULL;

CArchDaemonWindows::CArchDaemonWindows() :
	m_daemonThread(NULL)
{
	// do nothing
}

CArchDaemonWindows::~CArchDaemonWindows()
{
	// do nothing
}

int
CArchDaemonWindows::runDaemon(RunFunc runFunc)
{
	assert(s_daemon != NULL);

	return s_daemon->doRunDaemon(runFunc);
}

void
CArchDaemonWindows::daemonRunning(bool running)
{
	// if s_daemon is NULL we assume we're running on the windows
	// 95 family and we just ignore this call so the caller doesn't
	// have to go through the trouble of not calling it on the
	// windows 95 family.
	if (s_daemon != NULL) {
		s_daemon->doDaemonRunning(running);
	}
}

void
CArchDaemonWindows::daemonFailed(int result)
{
	// if s_daemon is NULL we assume we're running on the windows
	// 95 family and we just ignore this call so the caller doesn't
	// have to go through the trouble of not calling it on the
	// windows 95 family.
	if (s_daemon != NULL) {
		throw XArchDaemonRunFailed(result);
	}
}

void
CArchDaemonWindows::installDaemon(const char* name,
				const char* description,
				const char* pathname,
				const char* commandLine,
				bool allUsers)
{
	// if not for all users then use the user's autostart registry.
	// key.  if windows 95 family then use windows 95 services key.
	if (!allUsers || CArchMiscWindows::isWindows95Family()) {
		// open registry
		HKEY key = CArchMiscWindows::isWindows95Family() ?
							open95ServicesKey() : openUserStartupKey();
		if (key == NULL) {
			// can't open key
			throw XArchDaemonInstallFailed(new XArchEvalWindows);
		}

		// construct entry
		std::string value;
		value += "\"";
		value += pathname;
		value += "\" ";
		value += commandLine;

		// install entry
		CArchMiscWindows::setValue(key, name, value);

		// clean up
		CArchMiscWindows::closeKey(key);
	}

	// windows NT family services
	else {
		// open service manager
		SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
		if (mgr == NULL) {
			// can't open service manager
			throw XArchDaemonInstallFailed(new XArchEvalWindows);
		}

		// create the service
		SC_HANDLE service = CreateService(mgr,
								name,
								name,
								0,
								SERVICE_WIN32_OWN_PROCESS |
									SERVICE_INTERACTIVE_PROCESS,
								SERVICE_AUTO_START,
								SERVICE_ERROR_NORMAL,
								pathname,
								NULL,
								NULL,
								NULL,
								NULL,
								NULL);
		if (service == NULL) {
			// can't create service
			// FIXME -- handle ERROR_SERVICE_EXISTS
			DWORD err = GetLastError();
			CloseServiceHandle(mgr);
			throw XArchDaemonInstallFailed(new XArchEvalWindows(err));
		}

		// done with service and manager
		CloseServiceHandle(service);
		CloseServiceHandle(mgr);

		// open the registry key for this service
		HKEY key = openNTServicesKey();
		key      = CArchMiscWindows::openKey(key, name);
		if (key == NULL) {
			// can't open key
			DWORD err = GetLastError();
			try {
				uninstallDaemon(name, allUsers);
			}
			catch (...) {
				// ignore
			}
			throw XArchDaemonInstallFailed(new XArchEvalWindows(err));
		}

		// set the description
		CArchMiscWindows::setValue(key, _T("Description"), description);

		// set command line
		key = CArchMiscWindows::openKey(key, _T("Parameters"));
		if (key == NULL) {
			// can't open key
			DWORD err = GetLastError();
			CArchMiscWindows::closeKey(key);
			try {
				uninstallDaemon(name, allUsers);
			}
			catch (...) {
				// ignore
			}
			throw XArchDaemonInstallFailed(new XArchEvalWindows(err));
		}
		CArchMiscWindows::setValue(key, _T("CommandLine"), commandLine);

		// done with registry
		CArchMiscWindows::closeKey(key);
	}
}

void
CArchDaemonWindows::uninstallDaemon(const char* name, bool allUsers)
{
	// if not for all users then use the user's autostart registry.
	// key.  if windows 95 family then use windows 95 services key.
	if (!allUsers || CArchMiscWindows::isWindows95Family()) {
		// open registry
		HKEY key = CArchMiscWindows::isWindows95Family() ?
							open95ServicesKey() : openUserStartupKey();
		if (key == NULL) {
			// can't open key.  daemon is probably not installed.
			throw XArchDaemonUninstallNotInstalled(new XArchEvalWindows);
		}

		// remove entry
		CArchMiscWindows::deleteValue(key, name);

		// clean up
		CArchMiscWindows::closeKey(key);
	}

	// windows NT family services
	else {
		// remove parameters for this service.  ignore failures.
		HKEY key = openNTServicesKey();
		key      = CArchMiscWindows::openKey(key, name);
		if (key != NULL) {
			CArchMiscWindows::deleteKey(key, _T("Parameters"));
			CArchMiscWindows::closeKey(key);
		}

		// open service manager
		SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
		if (mgr == NULL) {
			// can't open service manager
			throw XArchDaemonUninstallFailed(new XArchEvalWindows);
		}

		// open the service.  oddly, you must open a service to delete it.
		SC_HANDLE service = OpenService(mgr, name, DELETE);
		if (service == NULL) {
			DWORD err = GetLastError();
			CloseServiceHandle(mgr);
			if (err != ERROR_SERVICE_DOES_NOT_EXIST) {
				throw XArchDaemonUninstallFailed(new XArchEvalWindows(err));
			}
			throw XArchDaemonUninstallNotInstalled(new XArchEvalWindows(err));
		}

		// delete the service
		const bool okay = (DeleteService(service) == 0);
		const DWORD err = GetLastError();

		// clean up
		CloseServiceHandle(service);
		CloseServiceHandle(mgr);

		// handle failure.  ignore error if service isn't installed anymore.
		if (!okay && isDaemonInstalled(name, allUsers)) {
			if (err != ERROR_SERVICE_MARKED_FOR_DELETE) {
				throw XArchDaemonUninstallFailed(new XArchEvalWindows(err));
			}
			throw XArchDaemonUninstallNotInstalled(new XArchEvalWindows(err));
		}
	}
}

int
CArchDaemonWindows::daemonize(const char* name, DaemonFunc func)
{
	assert(name != NULL);
	assert(func != NULL);

	// windows 95 family services
	if (CArchMiscWindows::isWindows95Family()) {
		typedef DWORD (WINAPI *RegisterServiceProcessT)(DWORD, DWORD);

		// mark this process as a service so it's not killed when the
		// user logs off.
		HINSTANCE kernel = LoadLibrary("kernel32.dll");
		if (kernel == NULL) {
			throw XArchDaemonFailed(new XArchEvalWindows);
		}
		RegisterServiceProcessT RegisterServiceProcess =
								reinterpret_cast<RegisterServiceProcessT>(
									GetProcAddress(kernel,
										"RegisterServiceProcess"));
		if (RegisterServiceProcess == NULL) {
			// missing RegisterServiceProcess function
			DWORD err = GetLastError();
			FreeLibrary(kernel);
			throw XArchDaemonFailed(new XArchEvalWindows(err));
		}
		if (RegisterServiceProcess(NULL, 1) == 0) {
			// RegisterServiceProcess failed
			DWORD err = GetLastError();
			FreeLibrary(kernel);
			throw XArchDaemonFailed(new XArchEvalWindows(err));
		}
		FreeLibrary(kernel);

		// now simply call the daemon function
		return func(1, &name);
	}

	// windows NT family services
	else {
		// save daemon function
		m_daemonFunc = func;

		// construct the service entry
		SERVICE_TABLE_ENTRY entry[2];
		entry[0].lpServiceName = const_cast<char*>(name);
		entry[0].lpServiceProc = &CArchDaemonWindows::serviceMainEntry;
		entry[1].lpServiceName = NULL;
		entry[1].lpServiceProc = NULL;

		// hook us up to the service control manager.  this won't return
		// (if successful) until the processes have terminated.
		s_daemon = this;
		if (StartServiceCtrlDispatcher(entry) == 0) {
			// StartServiceCtrlDispatcher failed
			s_daemon = NULL;
			throw XArchDaemonFailed(new XArchEvalWindows);
		}

		s_daemon = NULL;
		return m_daemonResult;
	}
}

bool
CArchDaemonWindows::canInstallDaemon(const char* name, bool allUsers)
{
	// if not for all users then use the user's autostart registry.
	// key.  if windows 95 family then use windows 95 services key.
	if (!allUsers || CArchMiscWindows::isWindows95Family()) {
		// check if we can open the registry key
		HKEY key = CArchMiscWindows::isWindows95Family() ?
							open95ServicesKey() : openUserStartupKey();
		CArchMiscWindows::closeKey(key);
		return (key != NULL);
	}

	// windows NT family services
	else {
		// check if we can open service manager for write
		SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
		if (mgr == NULL) {
			return false;
		}
		CloseServiceHandle(mgr);

		// check if we can open the registry key for this service
		HKEY key = openNTServicesKey();
		key      = CArchMiscWindows::openKey(key, name);
		key      = CArchMiscWindows::openKey(key, _T("Parameters"));
		CArchMiscWindows::closeKey(key);

		return (key != NULL);
	}
}

bool
CArchDaemonWindows::isDaemonInstalled(const char* name, bool allUsers)
{
	// if not for all users then use the user's autostart registry.
	// key.  if windows 95 family then use windows 95 services key.
	if (!allUsers || CArchMiscWindows::isWindows95Family()) {
		// check if we can open the registry key
		HKEY key = CArchMiscWindows::isWindows95Family() ?
							open95ServicesKey() : openUserStartupKey();
		if (key == NULL) {
			return false;
		}

		// check for entry
		const bool installed = !CArchMiscWindows::readValueString(key,
															name).empty();

		// clean up
		CArchMiscWindows::closeKey(key);

		return installed;
	}

	// windows NT family services
	else {
		// check parameters for this service
		HKEY key = openNTServicesKey();
		key      = CArchMiscWindows::openKey(key, name);
		key      = CArchMiscWindows::openKey(key, _T("Parameters"));
		if (key != NULL) {
			const bool installed = !CArchMiscWindows::readValueString(key,
										_T("CommandLine")).empty();
			CArchMiscWindows::closeKey(key);
			if (!installed) {
				return false;
			}
		}

		// open service manager
		SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_READ);
		if (mgr == NULL) {
			return false;
		}

		// open the service
		SC_HANDLE service = OpenService(mgr, name, GENERIC_READ);

		// clean up
		if (service != NULL) {
			CloseServiceHandle(service);
		}
		CloseServiceHandle(mgr);

		return (service != NULL);
	}
}

HKEY
CArchDaemonWindows::openNTServicesKey()
{
	static const char* s_keyNames[] = {
		_T("SYSTEM"),
		_T("CurrentControlSet"),
		_T("Services"),
		NULL
	};

	return CArchMiscWindows::openKey(HKEY_LOCAL_MACHINE, s_keyNames);
}

HKEY
CArchDaemonWindows::open95ServicesKey()
{
	static const char* s_keyNames[] = {
		_T("Software"),
		_T("Microsoft"),
		_T("Windows"),
		_T("CurrentVersion"),
		_T("RunServices"),
		NULL
	};

	return CArchMiscWindows::openKey(HKEY_LOCAL_MACHINE, s_keyNames);
}

HKEY
CArchDaemonWindows::openUserStartupKey()
{
	static const char* s_keyNames[] = {
		_T("Software"),
		_T("Microsoft"),
		_T("Windows"),
		_T("CurrentVersion"),
		_T("Run"),
		NULL
	};

	return CArchMiscWindows::openKey(HKEY_CURRENT_USER, s_keyNames);
}

int
CArchDaemonWindows::doRunDaemon(RunFunc run)
{
	// should only be called from DaemonFunc
	assert(m_serviceMutex != NULL);
	assert(run  != NULL);

	ARCH->lockMutex(m_serviceMutex);
	try {
		int result;
		m_serviceHandlerWaiting = false;
		m_serviceRunning        = false;
		for (;;) {
			// mark server as running
			setStatus(SERVICE_RUNNING);

			// run callback in another thread
			m_serviceRunning = true;
			m_daemonThread = ARCH->newThread(
							&CArchDaemonWindows::runDaemonThreadEntry, run);
			ARCH->wait(m_daemonThread, -1.0);
			result = reinterpret_cast<int>(
							ARCH->getResultOfThread(m_daemonThread));
			m_serviceRunning = false;

			// notify handler that the server stopped.  if handler
			// isn't waiting then we stopped unexpectedly and we
			// quit.
			if (m_serviceHandlerWaiting) {
				m_serviceHandlerWaiting = false;
				ARCH->broadcastCondVar(m_serviceCondVar);
			}
			else {
				break;
			}

			// wait until we're told what to do next
			while (m_serviceState != SERVICE_RUNNING &&
					m_serviceState != SERVICE_STOPPED) {
				ARCH->waitCondVar(m_serviceCondVar, m_serviceMutex, -1.0);
			}

			// exit loop if we've been told to stop
			if (m_serviceState == SERVICE_STOPPED) {
				break;
			}

			// done with callback thread
			ARCH->closeThread(m_daemonThread);
			m_daemonThread = NULL;
		}

		// prevent daemonHandler from changing state
		m_serviceState = SERVICE_STOPPED;

		// tell service control that the service is stopped.
		// FIXME -- hopefully this will ensure that our handler won't
		// be called again but i can't find documentation that
		// verifies that.  if it does it'll crash on the mutex that
		// we're about to destroy.
		setStatus(m_serviceState);

		// clean up
		if (m_daemonThread != NULL) {
			ARCH->closeThread(m_daemonThread);
			m_daemonThread = NULL;
		}
		ARCH->unlockMutex(m_serviceMutex);

		return result;
	}
	catch (...) {
		// FIXME -- report error

		// prevent serviceHandler from changing state
		m_serviceState = SERVICE_STOPPED;

		// set status
		setStatusError(0);

		// wake up serviceHandler if it's waiting then wait for it
		if (m_serviceHandlerWaiting) {
			m_serviceHandlerWaiting = false;
			ARCH->broadcastCondVar(m_serviceCondVar);
			ARCH->waitCondVar(m_serviceCondVar, m_serviceMutex, -1.0);
			// serviceHandler has exited by now
		}

		ARCH->unlockMutex(m_serviceMutex);
		throw;
	}
}

void
CArchDaemonWindows::doDaemonRunning(bool running)
{
	if (running) {
		ARCH->unlockMutex(m_serviceMutex);
	}
	else {
		ARCH->lockMutex(m_serviceMutex);
	}
}

void*
CArchDaemonWindows::runDaemonThread(RunFunc run)
{
	return reinterpret_cast<void*>(run());
}

void*
CArchDaemonWindows::runDaemonThreadEntry(void* vrun)
{
	assert(s_daemon != NULL);

	return s_daemon->runDaemonThread(reinterpret_cast<RunFunc>(vrun));
}

void
CArchDaemonWindows::setStatus(DWORD state)
{
	setStatus(state, 0, 0);
}

void
CArchDaemonWindows::setStatus(DWORD state, DWORD step, DWORD waitHint)
{
	assert(s_daemon != NULL);

	SERVICE_STATUS status;
	status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS |
										SERVICE_INTERACTIVE_PROCESS;
	status.dwCurrentState            = state;
	status.dwControlsAccepted        = SERVICE_ACCEPT_STOP |
										SERVICE_ACCEPT_PAUSE_CONTINUE |
										SERVICE_ACCEPT_SHUTDOWN;
	status.dwWin32ExitCode           = NO_ERROR;
	status.dwServiceSpecificExitCode = 0;
	status.dwCheckPoint              = step;
	status.dwWaitHint                = waitHint;
	SetServiceStatus(s_daemon->m_statusHandle, &status);
}

void
CArchDaemonWindows::setStatusError(DWORD error)
{
	SERVICE_STATUS status;
	status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS |
										SERVICE_INTERACTIVE_PROCESS;
	status.dwCurrentState            = SERVICE_STOPPED;
	status.dwControlsAccepted        = SERVICE_ACCEPT_STOP |
										SERVICE_ACCEPT_PAUSE_CONTINUE |
										SERVICE_ACCEPT_SHUTDOWN;
	status.dwWin32ExitCode           = ERROR_SERVICE_SPECIFIC_ERROR;
	status.dwServiceSpecificExitCode = error;
	status.dwCheckPoint              = 0;
	status.dwWaitHint                = 0;
	SetServiceStatus(s_daemon->m_statusHandle, &status);
}

void
CArchDaemonWindows::serviceMain(DWORD argc, LPTSTR* argvIn)
{
	typedef std::vector<LPCTSTR> ArgList;
	typedef std::vector<std::string> Arguments;
	const char** argv = const_cast<const char**>(argvIn);

	// create synchronization objects
	m_serviceMutex   = ARCH->newMutex();
	m_serviceCondVar = ARCH->newCondVar();
	m_serviceState   = SERVICE_RUNNING;

	// register our service handler functiom
	m_statusHandle = RegisterServiceCtrlHandler(argv[0],
								&CArchDaemonWindows::serviceHandlerEntry);
	if (m_statusHandle == NULL) {
		// cannot start as service
		m_daemonResult = -1;
		ARCH->closeCondVar(m_serviceCondVar);
		ARCH->closeMutex(m_serviceMutex);
		return;
	}

	// tell service control manager that we're starting
	setStatus(SERVICE_START_PENDING, 0, 10000);

	// if no arguments supplied then try getting them from the registry.
	// the first argument doesn't count because it's the service name.
	Arguments args;
	ArgList myArgv;
	if (argc <= 1) {
		// read command line
		std::string commandLine;
		HKEY key = openNTServicesKey();
		key      = CArchMiscWindows::openKey(key, argvIn[0]);
		key      = CArchMiscWindows::openKey(key, _T("Parameters"));
		if (key != NULL) {
			commandLine = CArchMiscWindows::readValueString(key,
												_T("CommandLine"));
		}

		// if the command line isn't empty then parse and use it
		if (!commandLine.empty()) {
			// parse, honoring double quoted substrings
			std::string::size_type i = commandLine.find_first_not_of(" \t");
			while (i != std::string::npos && i != commandLine.size()) {
				// find end of string
				std::string::size_type e;
				if (commandLine[i] == '\"') {
					// quoted.  find closing quote.
					++i;
					e = commandLine.find("\"", i);

					// whitespace must follow closing quote
					if (e == std::string::npos ||
						(e + 1 != commandLine.size() &&
						commandLine[e + 1] != ' ' &&
						commandLine[e + 1] != '\t')) {
						args.clear();
						break;
					}

					// extract
					args.push_back(commandLine.substr(i, e - i));
					i = e + 1;
				}
				else {
					// unquoted.  find next whitespace.
					e = commandLine.find_first_of(" \t", i);
					if (e == std::string::npos) {
						e = commandLine.size();
					}

					// extract
					args.push_back(commandLine.substr(i, e - i));
					i = e + 1;
				}

				// next argument
				i = commandLine.find_first_not_of(" \t", i);
			}

			// service name goes first
			myArgv.push_back(argv[0]);

			// get pointers
			for (size_t i = 0; i < args.size(); ++i) {
				myArgv.push_back(args[i].c_str());
			}

			// adjust argc/argv
			argc = myArgv.size();
			argv = &myArgv[0];
		}
	}

	try {
		// invoke daemon function
		m_daemonResult = m_daemonFunc(static_cast<int>(argc), argv);
	}
	catch (XArchDaemonRunFailed& e) {
		setStatusError(e.m_result);
		m_daemonResult = -1;
	}
	catch (...) {
		setStatusError(1);
		m_daemonResult = -1;
	}

	// clean up
	ARCH->closeCondVar(m_serviceCondVar);
	ARCH->closeMutex(m_serviceMutex);
}

void WINAPI
CArchDaemonWindows::serviceMainEntry(DWORD argc, LPTSTR* argv)
{
	s_daemon->serviceMain(argc, argv);
}

void
CArchDaemonWindows::serviceHandler(DWORD ctrl)
{
	assert(m_serviceMutex   != NULL);
	assert(m_serviceCondVar != NULL);

	ARCH->lockMutex(m_serviceMutex);

	// ignore request if service is already stopped
	if (m_serviceState == SERVICE_STOPPED) {
		setStatus(m_serviceState);
		ARCH->unlockMutex(m_serviceMutex);
		return;
	}

	switch (ctrl) {
	case SERVICE_CONTROL_PAUSE:
		// update state
		m_serviceState = SERVICE_PAUSE_PENDING;
		setStatus(m_serviceState, 0, 5000);

		// stop run callback if running and wait for it to finish
		if (m_serviceRunning) {
			m_serviceHandlerWaiting = true;
			ARCH->cancelThread(m_daemonThread);
			ARCH->waitCondVar(m_serviceCondVar, m_serviceMutex, -1.0);
		}

		// update state if service hasn't stopped while we were waiting
		if (m_serviceState != SERVICE_STOPPED) {
			m_serviceState = SERVICE_PAUSED;
		}
		ARCH->broadcastCondVar(m_serviceCondVar);
		break;

	case SERVICE_CONTROL_CONTINUE:
		// required status update
		setStatus(m_serviceState);

		// update state but let main loop send RUNNING notification
		m_serviceState = SERVICE_RUNNING;
		ARCH->broadcastCondVar(m_serviceCondVar);
		ARCH->unlockMutex(m_serviceMutex);
		return;

	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		// update state
		m_serviceState = SERVICE_STOP_PENDING;
		setStatus(m_serviceState, 0, 5000);

		// stop run callback if running and wait for it to finish
		if (m_serviceRunning) {
			m_serviceHandlerWaiting = true;
			ARCH->cancelThread(m_daemonThread);
			ARCH->waitCondVar(m_serviceCondVar, m_serviceMutex, -1.0);
		}

		// update state
		m_serviceState = SERVICE_STOPPED;
		ARCH->broadcastCondVar(m_serviceCondVar);
		break;

	default:
		// unknown service command
		// fall through

	case SERVICE_CONTROL_INTERROGATE:
		break;
	}

	// send update
	setStatus(m_serviceState);

	ARCH->unlockMutex(m_serviceMutex);
}

void WINAPI
CArchDaemonWindows::serviceHandlerEntry(DWORD ctrl)
{
	s_daemon->serviceHandler(ctrl);
}
