#include "CWin32Platform.h"
#include "CLock.h"
#include "CThread.h"
#include "CLog.h"
#include "TMethodJob.h"
#include "stdvector.h"
#include <cstring>
#include <shlobj.h>
#include <tchar.h>

//
// CWin32Platform
//

HANDLE					CWin32Platform::s_eventLog     = NULL;
CWin32Platform*			CWin32Platform::s_daemonPlatform = NULL;

CWin32Platform::CWin32Platform()
{
	// do nothing
}

CWin32Platform::~CWin32Platform()
{
	// do nothing
}

bool
CWin32Platform::isWindows95Family()
{
	OSVERSIONINFO version;
	version.dwOSVersionInfoSize = sizeof(version);
	if (GetVersionEx(&version) == 0) {
		log((CLOG_WARN "cannot determine OS: %d", GetLastError()));
		return true;
	}
	return (version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
}

void
CWin32Platform::setStatus(SERVICE_STATUS_HANDLE handle, DWORD state)
{
	setStatus(handle, state, 0, 0);
}

void
CWin32Platform::setStatus(SERVICE_STATUS_HANDLE handle,
				DWORD state, DWORD step, DWORD waitHint)
{
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
	SetServiceStatus(handle, &status);
}

void
CWin32Platform::setStatusError(SERVICE_STATUS_HANDLE handle, DWORD error)
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
	SetServiceStatus(handle, &status);
}

bool
CWin32Platform::installDaemon(const char* name, const char* description,
				const char* pathname, const char* commandLine)
{
	// windows 95 family services
	if (isWindows95Family()) {
		// open registry
		HKEY key = open95ServicesKey();
		if (key == NULL) {
			log((CLOG_ERR "cannot open RunServices registry key", GetLastError()));
			return false;
		}

		// construct entry
		CString value;
		value += "\"";
		value += pathname;
		value += "\" ";
		value += commandLine;

		// install entry
		setValue(key, name, value);

		// clean up
		closeKey(key);

		return true;
	}

	// windows NT family services
	else {
		// open service manager
		SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
		if (mgr == NULL) {
			log((CLOG_ERR "OpenSCManager failed with %d", GetLastError()));
			return false;
		}

		// create the servie
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

		// done with service and manager
		if (service != NULL) {
			CloseServiceHandle(service);
			CloseServiceHandle(mgr);
		}
		else {
			log((CLOG_ERR "CreateService failed with %d", GetLastError()));
			CloseServiceHandle(mgr);
			return false;
		}

		// open the registry key for this service
		HKEY key = openNTServicesKey();
		key      = openKey(key, name);
		if (key == NULL) {
			// can't open key
			uninstallDaemon(name);
			return false;
		}

		// set the description
		setValue(key, "Description", description);

		// set command line
		key = openKey(key, "Parameters");
		if (key == NULL) {
			// can't open key
			uninstallDaemon(name);
			return false;
		}
		setValue(key, "CommandLine", commandLine);

		// done with registry
		closeKey(key);

		return true;
	}
}

IPlatform::EResult
CWin32Platform::uninstallDaemon(const char* name)
{
	// windows 95 family services
	if (isWindows95Family()) {
		// open registry
		HKEY key = open95ServicesKey();
		if (key == NULL) {
			log((CLOG_ERR "cannot open RunServices registry key", GetLastError()));
			return kAlready;
		}

		// remove entry
		deleteValue(key, name);

		// clean up
		closeKey(key);

		return kSuccess;
	}

	// windows NT family services
	else {
		// remove parameters for this service
		HKEY key = openNTServicesKey();
		key      = openKey(key, name);
		if (key != NULL) {
			deleteKey(key, "Parameters");
			closeKey(key);
		}

		// open service manager
		SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
		if (mgr == NULL) {
			log((CLOG_ERR "OpenSCManager failed with %d", GetLastError()));
			return kFailed;
		}

		// open the service.  oddly, you must open a service to delete it.
		EResult result;
		SC_HANDLE service = OpenService(mgr, name, DELETE);
		if (service == NULL) {
			const DWORD e = GetLastError();
			log((CLOG_ERR "OpenService failed with %d", e));
			result = (e == ERROR_SERVICE_DOES_NOT_EXIST) ? kAlready : kFailed;
		}

		else {
			if (DeleteService(service) != 0) {
				result = kSuccess;
			}
			else {
				const DWORD e = GetLastError();
				switch (e) {
				case ERROR_SERVICE_MARKED_FOR_DELETE:
					result = kAlready;
					break;

				default:
					result = kFailed;
					break;
				}
			}
			CloseServiceHandle(service);
		}

		// close the manager
		CloseServiceHandle(mgr);

		return result;
	}
}

int
CWin32Platform::daemonize(const char* name, DaemonFunc func)
{
	assert(name != NULL);
	assert(func != NULL);

	// windows 95 family services
	if (isWindows95Family()) {
		typedef DWORD (WINAPI *RegisterServiceProcessT)(DWORD, DWORD);

		// mark this process as a service so it's not killed when the
		// user logs off.
		HINSTANCE kernel = LoadLibrary("kernel32.dll");
		if (kernel == NULL) {
			log((CLOG_ERR "LoadLibrary failed with %d", GetLastError()));
			return -1;
		}
		RegisterServiceProcessT RegisterServiceProcess =
								reinterpret_cast<RegisterServiceProcessT>(
									GetProcAddress(kernel,
										_T("RegisterServiceProcess")));
		if (RegisterServiceProcess == NULL) {
			log((CLOG_ERR "can't lookup RegisterServiceProcess: %d", GetLastError()));
			FreeLibrary(kernel);
			return -1;
		}
		if (RegisterServiceProcess(NULL, 1) == 0) {
			log((CLOG_ERR "RegisterServiceProcess failed with %d", GetLastError()));
			FreeLibrary(kernel);
			return -1;
		}
		FreeLibrary(kernel);

		// now simply call the daemon function
		return func(this, 1, &name);
	}

	// windows NT family services
	else {
		// save daemon function
		m_daemonFunc = func;

		// construct the service entry
		SERVICE_TABLE_ENTRY entry[2];
		entry[0].lpServiceName = const_cast<char*>(name);
		entry[0].lpServiceProc = &CWin32Platform::serviceMainEntry;
		entry[1].lpServiceName = NULL;
		entry[1].lpServiceProc = NULL;

		// hook us up to the service control manager.  this won't return
		// (if successful) until the processes have terminated.
		s_daemonPlatform = this;
		if (StartServiceCtrlDispatcher(entry)) {
			s_daemonPlatform = NULL;
			return m_daemonResult;
		}
		log((CLOG_ERR "StartServiceCtrlDispatcher failed with %d", GetLastError()));
		s_daemonPlatform = NULL;
		return -1;
	}
}

void
CWin32Platform::installDaemonLogger(const char* name)
{
	if (!CWin32Platform::isWindows95Family()) {
		// open event log and direct log messages to it
		if (s_eventLog == NULL) {
			s_eventLog = RegisterEventSource(NULL, name);
			if (s_eventLog != NULL) {
				CLog::setOutputter(&CWin32Platform::serviceLogger);
			}
		}
	}
}

const char*
CWin32Platform::getBasename(const char* pathname) const
{
	if (pathname == NULL) {
		return NULL;
	}

	// check for last /
	const char* basename = strrchr(pathname, '/');
	if (basename != NULL) {
		++basename;
	}
	else {
		basename = pathname;
	}

	// check for last backslash
	const char* basename2 = strrchr(pathname, '\\');
	if (basename2 != NULL && basename2 > basename) {
		basename = basename2 + 1;
	}

	return basename;
}

CString
CWin32Platform::getUserDirectory() const
{
	// try %HOMEPATH%
	TCHAR dir[MAX_PATH];
	DWORD size   = sizeof(dir) / sizeof(TCHAR);
	DWORD result = GetEnvironmentVariable(_T("HOMEPATH"), dir, size);
	if (result != 0 && result <= size) {
		// sanity check -- if dir doesn't appear to start with a
		// drive letter and isn't a UNC name then don't use it
		// FIXME -- allow UNC names
		if (dir[0] != '\0' && (dir[1] == ':' ||
			((dir[0] == '\\' || dir[0] == '/') &&
			(dir[1] == '\\' || dir[1] == '/')))) {
			return dir;
		}
	}

	// get the location of the personal files.  that's as close to
	// a home directory as we're likely to find.
	ITEMIDLIST* idl;
	if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &idl))) {
		TCHAR* path = NULL;
		if (SHGetPathFromIDList(idl, dir)) {
			DWORD attr = GetFileAttributes(dir);
			if (attr != 0xffffffff && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0)
				path = dir;
		}

		IMalloc* shalloc;
		if (SUCCEEDED(SHGetMalloc(&shalloc))) {
			shalloc->Free(idl);
			shalloc->Release();
		}

		if (path != NULL) {
			return path;
		}
	}

	// use root of C drive as a default
	return "C:";
}

CString
CWin32Platform::getSystemDirectory() const
{
	// get windows directory
	char dir[MAX_PATH];
	if (GetWindowsDirectory(dir, sizeof(dir)) != 0) {
		return dir;
	}
	else {
		// can't get it.  use C:\ as a default.
		return "C:";
	}
}

CString
CWin32Platform::addPathComponent(const CString& prefix,
				const CString& suffix) const
{
	CString path;
	path.reserve(prefix.size() + 1 + suffix.size());
	path += prefix;
	if (path.size() == 0 ||
		(path[path.size() - 1] != '\\' &&
		path[path.size() - 1] != '/')) {
		path += '\\';
	}
	path += suffix;
	return path;
}

HKEY
CWin32Platform::openKey(HKEY key, const char* keyName)
{
	// open next key
	HKEY newKey;
	LONG result = RegOpenKeyEx(key, keyName, 0,
								KEY_WRITE | KEY_QUERY_VALUE, &newKey);
	if (result != ERROR_SUCCESS) {
		DWORD disp;
		result = RegCreateKeyEx(key, keyName, 0, _T(""),
								0, KEY_WRITE | KEY_QUERY_VALUE,
								NULL, &newKey, &disp);
	}
	if (result != ERROR_SUCCESS) {
		RegCloseKey(key);
		return NULL;
	}

	// switch to new key
	RegCloseKey(key);
	return newKey;
}

HKEY
CWin32Platform::openKey(HKEY key, const char** keyNames)
{
	for (UInt32 i = 0; key != NULL && keyNames[i] != NULL; ++i) {
		// open next key
		key = openKey(key, keyNames[i]);
	}
	return key;
}

void
CWin32Platform::closeKey(HKEY key)
{
	assert(key  != NULL);
	RegCloseKey(key);
}

void
CWin32Platform::deleteKey(HKEY key, const char* name)
{
	assert(key  != NULL);
	assert(name != NULL);
	RegDeleteKey(key, name);
}

void
CWin32Platform::deleteValue(HKEY key, const char* name)
{
	assert(key  != NULL);
	assert(name != NULL);
	RegDeleteValue(key, name);
}

void
CWin32Platform::setValue(HKEY key, const char* name, const CString& value)
{
	assert(key  != NULL);
	assert(name != NULL);
	RegSetValueEx(key, name, 0, REG_SZ,
								reinterpret_cast<const BYTE*>(value.c_str()),
								value.size() + 1);
}

CString
CWin32Platform::readValueString(HKEY key, const char* name)
{
	// get the size of the string
	DWORD type;
	DWORD size = 0;
	LONG result = RegQueryValueEx(key, name, 0, &type, NULL, &size);
	if (result != ERROR_SUCCESS || type != REG_SZ) {
		return CString();
	}

	// allocate space
	char* buffer = new char[size];

	// read it
	result = RegQueryValueEx(key, name, 0, &type,
								reinterpret_cast<BYTE*>(buffer), &size);
	if (result != ERROR_SUCCESS || type != REG_SZ) {
		delete[] buffer;
		return CString();
	}

	// clean up and return value
	CString value(buffer);
	delete[] buffer;
	return value;
}

HKEY
CWin32Platform::openNTServicesKey()
{
	static const char* s_keyNames[] = {
		_T("SYSTEM"),
		_T("CurrentControlSet"),
		_T("Services"),
		NULL
	};

	return openKey(HKEY_LOCAL_MACHINE, s_keyNames);
}

HKEY
CWin32Platform::open95ServicesKey()
{
	static const char* s_keyNames[] = {
		_T("Software"),
		_T("Microsoft"),
		_T("Windows"),
		_T("CurrentVersion"),
		_T("RunServices"),
		NULL
	};

	return openKey(HKEY_LOCAL_MACHINE, s_keyNames);
}

int
CWin32Platform::runDaemon(RunFunc run, StopFunc stop)
{
	// should only be called from DaemonFunc
	assert(m_serviceMutex != NULL);

	CLock lock(m_serviceMutex);
	try {
		int result;
		m_stop                  = stop;
		m_serviceHandlerWaiting = false;
		m_serviceRunning        = false;
		for (;;) {
			// mark server as running
			setStatus(m_statusHandle, SERVICE_RUNNING);

			// run callback in another thread
			m_serviceRunning = true;
			{
				CThread thread(new TMethodJob<CWin32Platform>(this,
								&CWin32Platform::runDaemonThread, run));
				result = reinterpret_cast<int>(thread.getResult());
			}
			m_serviceRunning = false;

			// notify handler that the server stopped.  if handler
			// isn't waiting then we stopped unexpectedly and we
			// quit.
			if (m_serviceHandlerWaiting) {
				m_serviceHandlerWaiting = false;
				m_serviceState->broadcast();
			}
			else {
				break;
			}

			// wait until we're told what to do next
			while (*m_serviceState != SERVICE_RUNNING &&
					*m_serviceState != SERVICE_STOPPED) {
				m_serviceState->wait();
			}

			// exit loop if we've been told to stop
			if (*m_serviceState == SERVICE_STOPPED) {
				break;
			}
		}

		// prevent daemonHandler from changing state
		*m_serviceState = SERVICE_STOPPED;

		// tell service control that the service is stopped.
		// FIXME -- hopefully this will ensure that our handler won't
		// be called again but i can't find documentation that
		// verifies that.  if it does it'll crash on the mutex that
		// we're about to destroy.
		setStatus(m_statusHandle, *m_serviceState);

		// clean up
		m_stop = NULL;

		return result;
	}
	catch (...) {
		// FIXME -- report error

		// prevent serviceHandler from changing state
		*m_serviceState = SERVICE_STOPPED;

		// set status
		setStatusError(m_statusHandle, 0);

		// wake up serviceHandler if it's waiting then wait for it
		if (m_serviceHandlerWaiting) {
			m_serviceHandlerWaiting = false;
			m_serviceState->broadcast();
			m_serviceState->wait();
			// serviceHandler has exited by now
		}

		throw;
	}
}

void
CWin32Platform::runDaemonThread(void* vrun)
{
	RunFunc run = reinterpret_cast<RunFunc>(vrun);
	CThread::exit(reinterpret_cast<void*>(run(m_serviceMutex)));
}

void
CWin32Platform::serviceMain(DWORD argc, LPTSTR* argvIn)
{
	typedef std::vector<LPCTSTR> ArgList;
	typedef std::vector<CString> Arguments;
	const char** argv = const_cast<const char**>(argvIn);

	// open event log and direct log messages to it
	installDaemonLogger(argv[0]);

	// create synchronization objects
	CThread::init();
	m_serviceMutex = new CMutex;
	m_serviceState = new CCondVar<DWORD>(m_serviceMutex, SERVICE_RUNNING);

	// register our service handler functiom
	m_statusHandle = RegisterServiceCtrlHandler(argv[0],
								&CWin32Platform::serviceHandlerEntry);
	if (m_statusHandle == NULL) {
		// cannot start as service
		m_daemonResult = -1;
		delete m_serviceState;
		delete m_serviceMutex;
		return;
	}

	// tell service control manager that we're starting
	setStatus(m_statusHandle, SERVICE_START_PENDING, 0, 1000);

	// if no arguments supplied then try getting them from the registry.
	// the first argument doesn't count because it's the service name.
	Arguments args;
	ArgList myArgv;
	if (argc <= 1) {
		// read command line
		CString commandLine;
		HKEY key = openNTServicesKey();
		key      = openKey(key, argv[0]);
		key      = openKey(key, "Parameters");
		if (key != NULL) {
			commandLine = readValueString(key, "CommandLine");
		}

		// if the command line isn't empty then parse and use it
		if (!commandLine.empty()) {
			// parse, honoring double quoted substrings
			CString::size_type i = commandLine.find_first_not_of(" \t");
			while (i != CString::npos && i != commandLine.size()) {
				// find end of string
				CString::size_type e;
				if (commandLine[i] == '\"') {
					// quoted.  find closing quote.
					++i;
					e = commandLine.find("\"", i);

					// whitespace must follow closing quote
					if (e == CString::npos ||
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
					if (e == CString::npos) {
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
			for (UInt32 i = 0; i < args.size(); ++i) {
				myArgv.push_back(args[i].c_str());
			}

			// adjust argc/argv
			argc = myArgv.size();
			argv = &myArgv[0];
		}
	}

	try {
		// invoke daemon function
		m_daemonResult = m_daemonFunc(this, static_cast<int>(argc), argv);
	}
	catch (CDaemonFailed& e) {
		setStatusError(m_statusHandle, e.m_result);
		m_daemonResult = -1;
	}
	catch (...) {
		setStatusError(m_statusHandle, 1);
		m_daemonResult = -1;
	}

	// clean up
	delete m_serviceState;
	delete m_serviceMutex;

	// FIXME -- close event log?
}

void WINAPI
CWin32Platform::serviceMainEntry(DWORD argc, LPTSTR* argv)
{
	s_daemonPlatform->serviceMain(argc, argv);
}

void
CWin32Platform::serviceHandler(DWORD ctrl)
{
	assert(m_serviceMutex != NULL);
	assert(m_serviceState != NULL);

	CLock lock(m_serviceMutex);

	// ignore request if service is already stopped
	if (*m_serviceState == SERVICE_STOPPED) {
		setStatus(m_statusHandle, *m_serviceState);
		return;
	}

	switch (ctrl) {
	case SERVICE_CONTROL_PAUSE:
		// update state
		*m_serviceState = SERVICE_PAUSE_PENDING;
		setStatus(m_statusHandle, *m_serviceState, 0, 1000);

		// stop run callback if running and wait for it to finish
		if (m_serviceRunning) {
			m_serviceHandlerWaiting = true;
			m_stop();
			m_serviceState->wait();
		}

		// update state if service hasn't stopped while we were waiting
		if (*m_serviceState != SERVICE_STOPPED) {
			*m_serviceState = SERVICE_PAUSED;
		}
		m_serviceState->broadcast();
		break;

	case SERVICE_CONTROL_CONTINUE:
		// required status update
		setStatus(m_statusHandle, *m_serviceState);

		// update state but let main loop send RUNNING notification
		*m_serviceState = SERVICE_RUNNING;
		m_serviceState->broadcast();
		return;

	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		// update state
		*m_serviceState = SERVICE_STOP_PENDING;
		setStatus(m_statusHandle, *m_serviceState, 0, 1000);

		// stop run callback if running and wait for it to finish
		if (m_serviceRunning) {
			m_serviceHandlerWaiting = true;
			m_stop();
			m_serviceState->wait();
		}

		// update state
		*m_serviceState = SERVICE_STOPPED;
		m_serviceState->broadcast();
		break;

	default:
		log((CLOG_WARN "unknown service command: %d", ctrl));
		// fall through

	case SERVICE_CONTROL_INTERROGATE:
		break;
	}

	// send update
	setStatus(m_statusHandle, *m_serviceState);
}

void WINAPI
CWin32Platform::serviceHandlerEntry(DWORD ctrl)
{
	s_daemonPlatform->serviceHandler(ctrl);
}

bool
CWin32Platform::serviceLogger(int priority, const char* msg)
{
	if (s_eventLog == NULL) {
		return false;
	}

	// convert priority
	WORD type;
	switch (priority) {
	case CLog::kFATAL:
	case CLog::kERROR:
		type = EVENTLOG_ERROR_TYPE;
		break;

	case CLog::kWARNING:
		type = EVENTLOG_WARNING_TYPE;
		break;

	default:
		type = EVENTLOG_INFORMATION_TYPE;
		break;
	}

	// log it
	// FIXME -- win32 wants to use a message table to look up event
	// strings.  log messages aren't organized that way so we'll
	// just dump our string into the raw data section of the event
	// so users can at least see the message.  note that we use our
	// priority as the event category.
	ReportEvent(s_eventLog, type, static_cast<WORD>(priority),
								0,					// event ID
								NULL,
								0,
								strlen(msg + 1),	// raw data size
								NULL,
								const_cast<char*>(msg));// raw data
	return true;
}
