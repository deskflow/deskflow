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

#include "CMSWindowsScreenSaver.h"
#include "CMSWindowsScreen.h"
#include "CThread.h"
#include "CLog.h"
#include "TMethodJob.h"
#include "CArch.h"
#include "CArchMiscWindows.h"
#include <malloc.h>
#include <tchar.h>

#if !defined(SPI_GETSCREENSAVERRUNNING)
#define SPI_GETSCREENSAVERRUNNING 114
#endif

static const TCHAR* g_isSecureNT = "ScreenSaverIsSecure";
static const TCHAR* g_isSecure9x = "ScreenSaveUsePassword";
static const TCHAR* const g_pathScreenSaverIsSecure[] = {
	"Control Panel",
	"Desktop",
	NULL
};

//
// CMSWindowsScreenSaver
//

CMSWindowsScreenSaver::CMSWindowsScreenSaver() :
	m_wasSecure(false),
	m_wasSecureAnInt(false),
	m_process(NULL),
	m_watch(NULL),
	m_threadID(0),
	m_active(false)
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
	// if already started then say it didn't just start
	if (m_active) {
		return false;
	}

	// screen saver may have started.  look for it and get
	// the process.  if we can't find it then assume it
	// didn't really start.  we wait a moment before
	// looking to give the screen saver a chance to start.
	// this shouldn't be a problem since we only get here
	// if the screen saver wants to kick in, meaning that
	// the system is idle or the user deliberately started
	// the screen saver.
	Sleep(250);

	// set parameters common to all screen saver handling
	m_threadID = GetCurrentThreadId();
	m_msg      = msg;
	m_wParam   = wParam;
	m_lParam   = lParam;

	// we handle the screen saver differently for the windows
	// 95 and nt families.
	if (m_is95Family) {
		// on windows 95 we wait for the screen saver process
		// to terminate.  get the process.
		DWORD processID = findScreenSaver();
		HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, processID);
		if (process == NULL) {
			// didn't start
			LOG((CLOG_DEBUG "can't open screen saver process"));
			return false;
		}

		// watch for the process to exit
		watchProcess(process);
	}
	else {
		// on the windows nt family we wait for the desktop to
		// change until it's neither the Screen-Saver desktop
		// nor a desktop we can't open (the login desktop).
		// since windows will send the request-to-start-screen-
		// saver message even when the screen saver is disabled
		// we first check that the screen saver is indeed active
		// before watching for it to stop.
		if (!isActive()) {
			LOG((CLOG_DEBUG "can't open screen saver desktop"));
			return false;
		}

		watchDesktop();
	}

	return true;
}

void
CMSWindowsScreenSaver::enable()
{
	SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, m_wasEnabled, 0, 0);

	// restore password protection
	if (m_wasSecure) {
		setSecure(true, m_wasSecureAnInt);
	}

	// restore display power down
	CArchMiscWindows::removeBusyState(CArchMiscWindows::kDISPLAY);
}

void
CMSWindowsScreenSaver::disable()
{
	SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &m_wasEnabled, 0);
	SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, 0, 0);

	// disable password protected screensaver
	m_wasSecure = isSecure(&m_wasSecureAnInt);
	if (m_wasSecure) {
		setSecure(false, m_wasSecureAnInt);
	}

	// disable display power down
	CArchMiscWindows::addBusyState(CArchMiscWindows::kDISPLAY);
}

void
CMSWindowsScreenSaver::activate()
{
	// don't activate if already active
	if (!isActive()) {
		// activate
		HWND hwnd = GetForegroundWindow();
		if (hwnd != NULL) {
			PostMessage(hwnd, WM_SYSCOMMAND, SC_SCREENSAVE, 0);
		}
		else {
			// no foreground window.  pretend we got the event instead.
			DefWindowProc(NULL, WM_SYSCOMMAND, SC_SCREENSAVE, 0);
		}

		// restore power save when screen saver activates
		CArchMiscWindows::removeBusyState(CArchMiscWindows::kDISPLAY);
	}
}

void
CMSWindowsScreenSaver::deactivate()
{
	bool killed = false;
	if (!m_is95Family) {
		// NT runs screen saver in another desktop
		HDESK desktop = OpenDesktop("Screen-saver", 0, FALSE,
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
		if (hwnd == NULL) {
			// win2k may use a different class
			hwnd = FindWindow("Default Screen Saver", NULL);
		}
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

	// disable display power down
	CArchMiscWindows::removeBusyState(CArchMiscWindows::kDISPLAY);
}

bool
CMSWindowsScreenSaver::isActive() const
{
	if (m_is95) {
		return (FindWindow("WindowsScreenSaverClass", NULL) != NULL);
	}
	else if (m_isNT) {
		// screen saver runs on a separate desktop
		HDESK desktop = OpenDesktop("Screen-saver", 0, FALSE, MAXIMUM_ALLOWED);
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
		HINSTANCE instance = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);
		if (instance != CMSWindowsScreen::getInstance()) {
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			*reinterpret_cast<bool*>(arg) = true;
		}
	}
	return TRUE;
}

DWORD
CMSWindowsScreenSaver::findScreenSaver()
{
	// try windows 95 way
	HWND hwnd = FindWindow("WindowsScreenSaverClass", NULL);

	// get process ID of process that owns the window, if found
	if (hwnd != NULL) {
		DWORD processID;
		GetWindowThreadProcessId(hwnd, &processID);
		return processID;
	}

	// not found
	return 0;
}

void
CMSWindowsScreenSaver::watchDesktop()
{
	// stop watching previous process/desktop
	unwatchProcess();

	// watch desktop in another thread
	LOG((CLOG_DEBUG "watching screen saver desktop"));
	m_active = true;
	m_watch  = new CThread(new TMethodJob<CMSWindowsScreenSaver>(this,
								&CMSWindowsScreenSaver::watchDesktopThread));
}

void
CMSWindowsScreenSaver::watchProcess(HANDLE process)
{
	// stop watching previous process/desktop
	unwatchProcess();

	// watch new process in another thread
	if (process != NULL) {
		LOG((CLOG_DEBUG "watching screen saver process"));
		m_process = process;
		m_active  = true;
		m_watch   = new CThread(new TMethodJob<CMSWindowsScreenSaver>(this,
								&CMSWindowsScreenSaver::watchProcessThread));
	}
}

void
CMSWindowsScreenSaver::unwatchProcess()
{
	if (m_watch != NULL) {
		LOG((CLOG_DEBUG "stopped watching screen saver process/desktop"));
		m_watch->cancel();
		m_watch->wait();
		delete m_watch;
		m_watch  = NULL;
		m_active = false;
	}
	if (m_process != NULL) {
		CloseHandle(m_process);
		m_process = NULL;
	}
}

void
CMSWindowsScreenSaver::watchDesktopThread(void*)
{
	DWORD reserved = 0;
	TCHAR* name    = NULL;

	for (;;) {
		// wait a bit
		ARCH->sleep(0.2);

		if (m_isNT) {
			// get current desktop
			HDESK desk = OpenInputDesktop(0, FALSE, GENERIC_READ);
			if (desk == NULL) {
				// can't open desktop so keep waiting
				continue;
			}

			// get current desktop name length
			DWORD size;
			GetUserObjectInformation(desk, UOI_NAME, NULL, 0, &size);

			// allocate more space for the name, if necessary
			if (size > reserved) {
				reserved = size;
				name     = (TCHAR*)alloca(reserved + sizeof(TCHAR));
			}

			// get current desktop name
			GetUserObjectInformation(desk, UOI_NAME, name, size, &size);
			CloseDesktop(desk);

			// compare name to screen saver desktop name
			if (_tcsicmp(name, TEXT("Screen-saver")) == 0) {
				// still the screen saver desktop so keep waiting
				continue;
			}
		}
		else {
			// 2000/XP have a sane way to detect a runnin screensaver.
			BOOL running;
			SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &running, 0);
			if (running) {
				continue;
			}
		}

		// send screen saver deactivation message
		m_active = false;
		PostThreadMessage(m_threadID, m_msg, m_wParam, m_lParam);
		return;
	}
}

void
CMSWindowsScreenSaver::watchProcessThread(void*)
{
	for (;;) {
		CThread::testCancel();
		if (WaitForSingleObject(m_process, 50) == WAIT_OBJECT_0) {
			// process terminated
			LOG((CLOG_DEBUG "screen saver died"));

			// send screen saver deactivation message
			m_active = false;
			PostThreadMessage(m_threadID, m_msg, m_wParam, m_lParam);
			return;
		}
	}
}

void
CMSWindowsScreenSaver::setSecure(bool secure, bool saveSecureAsInt)
{
	HKEY hkey =
		CArchMiscWindows::openKey(HKEY_CURRENT_USER, g_pathScreenSaverIsSecure);
	if (hkey == NULL) {
		return;
	}

	const TCHAR* isSecure = m_is95Family ? g_isSecure9x : g_isSecureNT;
	if (saveSecureAsInt) {
		CArchMiscWindows::setValue(hkey, isSecure, secure ? 1 : 0);
	}
	else {
		CArchMiscWindows::setValue(hkey, isSecure, secure ? "1" : "0");
	}

	CArchMiscWindows::closeKey(hkey);
}

bool
CMSWindowsScreenSaver::isSecure(bool* wasSecureFlagAnInt) const
{
	// get the password protection setting key
	HKEY hkey =
		CArchMiscWindows::openKey(HKEY_CURRENT_USER, g_pathScreenSaverIsSecure);
	if (hkey == NULL) {
		return false;
	}

	// get the value.  the value may be an int or a string, depending
	// on the version of windows.
	bool result;
	const TCHAR* isSecure = m_is95Family ? g_isSecure9x : g_isSecureNT;
	switch (CArchMiscWindows::typeOfValue(hkey, isSecure)) {
	default:
		result = false;
		break;

	case CArchMiscWindows::kUINT: {
		DWORD value =
			CArchMiscWindows::readValueInt(hkey, isSecure);
		*wasSecureFlagAnInt = true;
		result = (value != 0);
		break;
	}

	case CArchMiscWindows::kSTRING: {
		std::string value =
			CArchMiscWindows::readValueString(hkey, isSecure);
		*wasSecureFlagAnInt = false;
		result = (value != "0");
		break;
	}
	}

	CArchMiscWindows::closeKey(hkey);
	return result;
}
