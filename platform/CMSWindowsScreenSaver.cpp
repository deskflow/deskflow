#include "CMSWindowsScreenSaver.h"
#include "CThread.h"
#include "CLog.h"
#include "TMethodJob.h"

#if !defined(SPI_GETSCREENSAVERRUNNING)
#define SPI_GETSCREENSAVERRUNNING 114
#endif

//
// CMSWindowsScreenSaver
//

CMSWindowsScreenSaver::CMSWindowsScreenSaver() :
	m_process(NULL),
	m_threadID(0),
	m_watch(NULL)
{
	// detect OS
	m_is95Family = false;
	m_is95       = false;
	m_isNT       = false;
	OSVERSIONINFO info;
	info.dwOSVersionInfoSize = sizeof(info);
	if (GetVersionEx(&info)) {
		m_is95Family = (info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
		if (info.dwPlatformId   == VER_PLATFORM_WIN32_NT &&
			info.dwMajorVersion <= 4) {
			m_isNT = true;
		}
		else if (info.dwPlatformId  == VER_PLATFORM_WIN32_WINDOWS &&
				info.dwMajorVersion == 4 &&
				info.dwMinorVersion == 0) {
			m_is95 = true;
		}
	}

	// check if screen saver is enabled
	SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &m_wasEnabled, 0);
}

CMSWindowsScreenSaver::~CMSWindowsScreenSaver()
{
	unwatchProcess();
}

bool
CMSWindowsScreenSaver::checkStarted(UINT msg, WPARAM wParam, LPARAM lParam)
{
	// screen saver may have started.  look for it and get
	// the process.  if we can't find it then assume it
	// didn't really start.  we wait a moment before
	// looking to give the screen saver a chance to start.
	// this shouldn't be a problem since we only get here
	// if the screen saver wants to kick in, meaning that
	// the system is idle or the user deliberately started
	// the screen saver.
	Sleep(250);
	HWND hwnd = findScreenSaver();
	if (hwnd == NULL) {
		// didn't start
		log((CLOG_DEBUG "can't find screen saver window"));
		return false;
	}

	// get the process
	DWORD processID;
	GetWindowThreadProcessId(hwnd, &processID);
	HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, processID);
	if (process == NULL) {
		// didn't start
		log((CLOG_DEBUG "can't open screen saver process"));
		return false;
	}

	// watch for the process to exit
	m_threadID = GetCurrentThreadId();
	m_msg      = msg;
	m_wParam   = wParam;
	m_lParam   = lParam;
	watchProcess(process);

	return true;
}

void
CMSWindowsScreenSaver::enable()
{
	SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, m_wasEnabled, 0, 0);
}

void
CMSWindowsScreenSaver::disable()
{
	SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &m_wasEnabled, 0);
	SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, 0, 0);
}

void
CMSWindowsScreenSaver::activate()
{
	// don't activate if already active
	if (!isActive()) {
		HWND hwnd = GetForegroundWindow();
		if (hwnd != NULL) {
			PostMessage(hwnd, WM_SYSCOMMAND, SC_SCREENSAVE, 0);
		}
		else {
			// no foreground window.  pretend we got the event instead.
			DefWindowProc(NULL, WM_SYSCOMMAND, SC_SCREENSAVE, 0);
		}
	}
}

void
CMSWindowsScreenSaver::deactivate()
{
	bool killed = false;
	if (!m_is95Family) {
		// NT runs screen saver in another desktop
		HDESK desktop = OpenDesktop("screen-saver", 0, FALSE,
								DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS);
		if (desktop != NULL) {
			EnumDesktopWindows(desktop,
								&CMSWindowsScreenSaver::killScreenSaverFunc,
								reinterpret_cast<LPARAM>(&killed));
			CloseDesktop(desktop);
		}
	}

	// if above failed or wasn't tried, try the windows 95 way
	if (!killed) {
		// find screen saver window and close it
		HWND hwnd = FindWindow("WindowsScreenSaverClass", NULL);
		if (hwnd != NULL) {
			PostMessage(hwnd, WM_CLOSE, 0, 0);
		}
	}

	// force timer to restart
	SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &m_wasEnabled, 0);
	SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,
								!m_wasEnabled, 0, SPIF_SENDWININICHANGE);
	SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,
								 m_wasEnabled, 0, SPIF_SENDWININICHANGE);
}

bool
CMSWindowsScreenSaver::isActive() const
{
	if (m_is95) {
		return (FindWindow("WindowsScreenSaverClass", NULL) != NULL);
	}
	else if (m_isNT) {
		// screen saver runs on a separate desktop
		HDESK desktop = OpenDesktop("screen-saver", 0, FALSE, MAXIMUM_ALLOWED);
		if (desktop == NULL && GetLastError() != ERROR_ACCESS_DENIED) {
			// desktop doesn't exist so screen saver is not running
			return false;
		}

		// desktop exists.  this should indicate that the screen saver
		// is running but an OS bug can cause a valid handle to be
		// returned even if the screen saver isn't running (Q230117).
		// we'll try to enumerate the windows on the desktop and, if
		// there are any, we assume the screen saver is running.  (note
		// that if we don't have permission to enumerate then we'll
		// assume that the screen saver is not running.)  that'd be
		// easy enough except there's another OS bug (Q198590) that can
		// cause EnumDesktopWindows() to enumerate the windows of
		// another desktop if the requested desktop has no windows.  to
		// work around that we have to verify that the enumerated
		// windows are, in fact, on the expected desktop.
		CFindScreenSaverInfo info;
		info.m_desktop = desktop;
		info.m_window  = NULL;
		EnumDesktopWindows(desktop,
								&CMSWindowsScreenSaver::findScreenSaverFunc,
								reinterpret_cast<LPARAM>(&info));

		// done with desktop
		CloseDesktop(desktop);

		// screen saver is running if a window was found
		return (info.m_window != NULL);
	}
	else {
		BOOL running;
		SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &running, 0);
		return (running != FALSE);
	}
}

BOOL CALLBACK
CMSWindowsScreenSaver::findScreenSaverFunc(HWND hwnd, LPARAM arg)
{
	CFindScreenSaverInfo* info = reinterpret_cast<CFindScreenSaverInfo*>(arg);

	if (info->m_desktop != NULL) {
		DWORD threadID = GetWindowThreadProcessId(hwnd, NULL);
		HDESK desktop  = GetThreadDesktop(threadID);
		if (desktop != NULL && desktop != info->m_desktop) {
			// stop enumerating -- wrong desktop
			return FALSE;
		}
	}

	// found a window
	info->m_window = hwnd;

	// don't need to enumerate further
	return FALSE;
}

BOOL CALLBACK
CMSWindowsScreenSaver::killScreenSaverFunc(HWND hwnd, LPARAM arg)
{
	if (IsWindowVisible(hwnd)) {
		PostMessage(hwnd, WM_CLOSE, 0, 0);
		*reinterpret_cast<bool*>(arg) = true;
	}
	return TRUE;
}

HWND
CMSWindowsScreenSaver::findScreenSaver()
{
	if (!m_is95Family) {
		// screen saver runs on a separate desktop
		HDESK desktop = OpenDesktop("screen-saver", 0, FALSE, MAXIMUM_ALLOWED);
		if (desktop != NULL) {
			// search
			CFindScreenSaverInfo info;
			info.m_desktop = desktop;
			info.m_window  = NULL;
			EnumDesktopWindows(desktop,
								&CMSWindowsScreenSaver::findScreenSaverFunc,
								reinterpret_cast<LPARAM>(&info));

			// done with desktop
			CloseDesktop(desktop);

			// return window if found
			if (info.m_window != NULL) {
				return info.m_window;
			}
		}
	}

	// try windows 95 way
	return FindWindow("WindowsScreenSaverClass", NULL);
}

void
CMSWindowsScreenSaver::watchProcess(HANDLE process)
{
	// stop watching previous process
	unwatchProcess();

	// watch new process in another thread
	if (process != NULL) {
		log((CLOG_DEBUG "watching screen saver process"));
		m_process = process;
		m_watch   = new CThread(new TMethodJob<CMSWindowsScreenSaver>(this,
								&CMSWindowsScreenSaver::watchProcessThread));
	}
}

void
CMSWindowsScreenSaver::unwatchProcess()
{
	if (m_watch != NULL) {
		log((CLOG_DEBUG "stopped watching screen saver process"));
		m_watch->cancel();
		m_watch->wait();
		delete m_watch;
		m_watch = NULL;
	}
	if (m_process != NULL) {
		CloseHandle(m_process);
		m_process = NULL;
	}
}

void
CMSWindowsScreenSaver::watchProcessThread(void*)
{
	for (;;) {
		CThread::testCancel();
		if (WaitForSingleObject(m_process, 50) == WAIT_OBJECT_0) {
			// process terminated.  send screen saver deactivation
			// message.
			log((CLOG_DEBUG "screen saver died"));
			PostThreadMessage(m_threadID, m_msg, m_wParam, m_lParam);
			return;
		}
	}
}
