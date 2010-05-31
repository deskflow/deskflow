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

#include "CArchMiscWindows.h"
#include "CArchDaemonWindows.h"
#include "CLog.h"

#include <Wtsapi32.h>
#pragma warning(disable: 4099)
#include <Userenv.h>
#pragma warning(default: 4099)
#include "Version.h"

// parent process name for services in Vista
#define SERVICE_LAUNCHER "services.exe"

#ifndef ES_SYSTEM_REQUIRED
#define ES_SYSTEM_REQUIRED  ((DWORD)0x00000001)
#endif
#ifndef ES_DISPLAY_REQUIRED
#define ES_DISPLAY_REQUIRED ((DWORD)0x00000002)
#endif
#ifndef ES_CONTINUOUS
#define ES_CONTINUOUS       ((DWORD)0x80000000)
#endif
typedef DWORD EXECUTION_STATE;

//
// CArchMiscWindows
//

CArchMiscWindows::CDialogs* CArchMiscWindows::s_dialogs   = NULL;
DWORD						CArchMiscWindows::s_busyState = 0;
CArchMiscWindows::STES_t	CArchMiscWindows::s_stes      = NULL;
HICON						CArchMiscWindows::s_largeIcon = NULL;
HICON						CArchMiscWindows::s_smallIcon = NULL;
HINSTANCE					CArchMiscWindows::s_instanceWin32 = NULL;

void
CArchMiscWindows::init()
{
	s_dialogs = new CDialogs;
	isWindows95Family();
}

bool
CArchMiscWindows::isWindows95Family()
{
	static bool init   = false;
	static bool result = false;

	if (!init) {
		OSVERSIONINFO version;
		version.dwOSVersionInfoSize = sizeof(version);
		if (GetVersionEx(&version) == 0) {
			// cannot determine OS;  assume windows 95 family
			result = true;
		}
		else {
			result = (version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
		}
		init = true;
	}
	return result;
}

bool
CArchMiscWindows::isWindowsModern()
{
	static bool init   = false;
	static bool result = false;

	if (!init) {
		OSVERSIONINFO version;
		version.dwOSVersionInfoSize = sizeof(version);
		if (GetVersionEx(&version) == 0) {
			// cannot determine OS;  assume not modern
			result = false;
		}
		else {
			result = ((version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
						version.dwMajorVersion == 4 &&
						version.dwMinorVersion > 0) ||
						(version.dwPlatformId == VER_PLATFORM_WIN32_NT &&
						version.dwMajorVersion > 4));
		}
		init = true;
	}
	return result;
}

void
CArchMiscWindows::setIcons(HICON largeIcon, HICON smallIcon)
{
	s_largeIcon = largeIcon;
	s_smallIcon = smallIcon;
}

void
CArchMiscWindows::getIcons(HICON& largeIcon, HICON& smallIcon)
{
	largeIcon = s_largeIcon;
	smallIcon = s_smallIcon;
}

int
CArchMiscWindows::runDaemon(RunFunc runFunc)
{
	return CArchDaemonWindows::runDaemon(runFunc);
}

void
CArchMiscWindows::daemonRunning(bool running)
{
	CArchDaemonWindows::daemonRunning(running);
}

void
CArchMiscWindows::daemonFailed(int result)
{
	CArchDaemonWindows::daemonFailed(result);
}

UINT
CArchMiscWindows::getDaemonQuitMessage()
{
	return CArchDaemonWindows::getDaemonQuitMessage();
}

HKEY
CArchMiscWindows::openKey(HKEY key, const TCHAR* keyName)
{
	return openKey(key, keyName, false);
}

HKEY
CArchMiscWindows::openKey(HKEY key, const TCHAR* const* keyNames)
{
	return openKey(key, keyNames, false);
}

HKEY
CArchMiscWindows::addKey(HKEY key, const TCHAR* keyName)
{
	return openKey(key, keyName, true);
}

HKEY
CArchMiscWindows::addKey(HKEY key, const TCHAR* const* keyNames)
{
	return openKey(key, keyNames, true);
}

HKEY
CArchMiscWindows::openKey(HKEY key, const TCHAR* keyName, bool create)
{
	// ignore if parent is NULL
	if (key == NULL) {
		return NULL;
	}

	// open next key
	HKEY newKey;
	LONG result = RegOpenKeyEx(key, keyName, 0,
								KEY_WRITE | KEY_QUERY_VALUE, &newKey);
	if (result != ERROR_SUCCESS && create) {
		DWORD disp;
		result = RegCreateKeyEx(key, keyName, 0, TEXT(""),
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
CArchMiscWindows::openKey(HKEY key, const TCHAR* const* keyNames, bool create)
{
	for (size_t i = 0; key != NULL && keyNames[i] != NULL; ++i) {
		// open next key
		key = openKey(key, keyNames[i], create);
	}
	return key;
}

void
CArchMiscWindows::closeKey(HKEY key)
{
	assert(key  != NULL);
	if (key==NULL) return;
	RegCloseKey(key);
}

void
CArchMiscWindows::deleteKey(HKEY key, const TCHAR* name)
{
	assert(key  != NULL);
	assert(name != NULL);
	if (key==NULL || name==NULL) return;
	RegDeleteKey(key, name);
}

void
CArchMiscWindows::deleteValue(HKEY key, const TCHAR* name)
{
	assert(key  != NULL);
	assert(name != NULL);
	if (key==NULL || name==NULL) return;
	RegDeleteValue(key, name);
}

bool
CArchMiscWindows::hasValue(HKEY key, const TCHAR* name)
{
	DWORD type;
	LONG result = RegQueryValueEx(key, name, 0, &type, NULL, NULL);
	return (result == ERROR_SUCCESS &&
			(type == REG_DWORD || type == REG_SZ));
}

CArchMiscWindows::EValueType
CArchMiscWindows::typeOfValue(HKEY key, const TCHAR* name)
{
	DWORD type;
	LONG result = RegQueryValueEx(key, name, 0, &type, NULL, NULL);
	if (result != ERROR_SUCCESS) {
		return kNO_VALUE;
	}
	switch (type) {
	case REG_DWORD:
		return kUINT;

	case REG_SZ:
		return kSTRING;

	case REG_BINARY:
		return kBINARY;

	default:
		return kUNKNOWN;
	}
}

void
CArchMiscWindows::setValue(HKEY key,
				const TCHAR* name, const std::string& value)
{
	assert(key  != NULL);
	assert(name != NULL);
	if(key ==NULL || name==NULL) return; // TODO: throw exception
	RegSetValueEx(key, name, 0, REG_SZ,
								reinterpret_cast<const BYTE*>(value.c_str()),
								(DWORD)value.size() + 1);
}

void
CArchMiscWindows::setValue(HKEY key, const TCHAR* name, DWORD value)
{
	assert(key  != NULL);
	assert(name != NULL);
	if(key ==NULL || name==NULL) return; // TODO: throw exception
	RegSetValueEx(key, name, 0, REG_DWORD,
								reinterpret_cast<CONST BYTE*>(&value),
								sizeof(DWORD));
}

void
CArchMiscWindows::setValueBinary(HKEY key,
				const TCHAR* name, const std::string& value)
{
	assert(key  != NULL);
	assert(name != NULL);
	if(key ==NULL || name==NULL) return; // TODO: throw exception
	RegSetValueEx(key, name, 0, REG_BINARY,
								reinterpret_cast<const BYTE*>(value.data()),
								(DWORD)value.size());
}

std::string
CArchMiscWindows::readBinaryOrString(HKEY key, const TCHAR* name, DWORD type)
{
	// get the size of the string
	DWORD actualType;
	DWORD size = 0;
	LONG result = RegQueryValueEx(key, name, 0, &actualType, NULL, &size);
	if (result != ERROR_SUCCESS || actualType != type) {
		return std::string();
	}

	// if zero size then return empty string
	if (size == 0) {
		return std::string();
	}

	// allocate space
	char* buffer = new char[size];

	// read it
	result = RegQueryValueEx(key, name, 0, &actualType,
								reinterpret_cast<BYTE*>(buffer), &size);
	if (result != ERROR_SUCCESS || actualType != type) {
		delete[] buffer;
		return std::string();
	}

	// clean up and return value
	if (type == REG_SZ && buffer[size - 1] == '\0') {
		// don't include terminating nul;  std::string will add one.
		--size;
	}
	std::string value(buffer, size);
	delete[] buffer;
	return value;
}

std::string
CArchMiscWindows::readValueString(HKEY key, const TCHAR* name)
{
	return readBinaryOrString(key, name, REG_SZ);
}

std::string
CArchMiscWindows::readValueBinary(HKEY key, const TCHAR* name)
{
	return readBinaryOrString(key, name, REG_BINARY);
}

DWORD
CArchMiscWindows::readValueInt(HKEY key, const TCHAR* name)
{
	DWORD type;
	DWORD value;
	DWORD size = sizeof(value);
	LONG result = RegQueryValueEx(key, name, 0, &type,
								reinterpret_cast<BYTE*>(&value), &size);
	if (result != ERROR_SUCCESS || type != REG_DWORD) {
		return 0;
	}
	return value;
}

void
CArchMiscWindows::addDialog(HWND hwnd)
{
	s_dialogs->insert(hwnd);
}

void
CArchMiscWindows::removeDialog(HWND hwnd)
{
	s_dialogs->erase(hwnd);
}

bool
CArchMiscWindows::processDialog(MSG* msg)
{
	for (CDialogs::const_iterator index = s_dialogs->begin();
							index != s_dialogs->end(); ++index) {
		if (IsDialogMessage(*index, msg)) {
			return true;
		}
	}
	return false;
}

void
CArchMiscWindows::addBusyState(DWORD busyModes)
{
	s_busyState |= busyModes;
	setThreadExecutionState(s_busyState);
}

void
CArchMiscWindows::removeBusyState(DWORD busyModes)
{
	s_busyState &= ~busyModes;
	setThreadExecutionState(s_busyState);
}

void
CArchMiscWindows::setThreadExecutionState(DWORD busyModes)
{
	// look up function dynamically so we work on older systems
	if (s_stes == NULL) {
		HINSTANCE kernel = LoadLibrary("kernel32.dll");
		if (kernel != NULL) {
			s_stes = reinterpret_cast<STES_t>(GetProcAddress(kernel,
							"SetThreadExecutionState"));
		}
		if (s_stes == NULL) {
			s_stes = &CArchMiscWindows::dummySetThreadExecutionState;
		}
	}

	// convert to STES form
	EXECUTION_STATE state = 0;
	if ((busyModes & kSYSTEM) != 0) {
		state |= ES_SYSTEM_REQUIRED;
	}
	if ((busyModes & kDISPLAY) != 0) {
		state |= ES_DISPLAY_REQUIRED;
	}
	if (state != 0) {
		state |= ES_CONTINUOUS;
	}

	// do it
	s_stes(state);
}

DWORD
CArchMiscWindows::dummySetThreadExecutionState(DWORD)
{
	// do nothing
	return 0;
}

void
CArchMiscWindows::wakeupDisplay()
{
	// We can't use ::setThreadExecutionState here because it sets
	// ES_CONTINUOUS, which we don't want.

	if (s_stes == NULL) {
		HINSTANCE kernel = LoadLibrary("kernel32.dll");
		if (kernel != NULL) {
			s_stes = reinterpret_cast<STES_t>(GetProcAddress(kernel,
							"SetThreadExecutionState"));
		}
		if (s_stes == NULL) {
			s_stes = &CArchMiscWindows::dummySetThreadExecutionState;
		}
	}

	s_stes(ES_DISPLAY_REQUIRED);

	// restore the original execution states
	setThreadExecutionState(s_busyState);
}

bool
CArchMiscWindows::wasLaunchedAsService() 
{
	CString name;
	if (!getParentProcessName(name)) {
		LOG((CLOG_ERR "cannot determine if process was launched as service"));
		return false;
	}

	return (name == SERVICE_LAUNCHER);
}

bool
CArchMiscWindows::getParentProcessName(CString &name) 
{	
	PROCESSENTRY32 parentEntry;
	if (!getParentProcessEntry(parentEntry)){
		LOG((CLOG_ERR "could not get entry for parent process"));
		return false;
	}

	name = parentEntry.szExeFile;
	return true;
}

BOOL WINAPI 
CArchMiscWindows::getSelfProcessEntry(PROCESSENTRY32& entry)
{
	// get entry from current PID
	return getProcessEntry(entry, GetCurrentProcessId());
}

BOOL WINAPI 
CArchMiscWindows::getParentProcessEntry(PROCESSENTRY32& entry)
{
	// get the current process, so we can get parent PID
	PROCESSENTRY32 selfEntry;
	if (!getSelfProcessEntry(selfEntry)) {
		return FALSE;
	}

	// get entry from parent PID
	return getProcessEntry(entry, selfEntry.th32ParentProcessID);
}

BOOL WINAPI 
CArchMiscWindows::getProcessEntry(PROCESSENTRY32& entry, DWORD processID)
{
	// first we need to take a snapshot of the running processes
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) {
		LOG((CLOG_ERR "could not get process snapshot (error: %i)", 
			GetLastError()));
		return FALSE;
	}

	entry.dwSize = sizeof(PROCESSENTRY32);

	// get the first process, and if we can't do that then it's 
	// unlikely we can go any further
	BOOL gotEntry = Process32First(snapshot, &entry);
	if (!gotEntry) {
		LOG((CLOG_ERR "could not get first process entry (error: %i)", 
			GetLastError()));
		return FALSE;
	}

	while(gotEntry) {

		if (entry.th32ProcessID == processID) {
			// found current process
			return TRUE;
		}

		// now move on to the next entry (when we reach end, loop will stop)
		gotEntry = Process32Next(snapshot, &entry);
	}

	return FALSE;
}

HINSTANCE
CArchMiscWindows::instanceWin32()
{
	assert(s_instanceWin32 != NULL);
	return s_instanceWin32;
}

void
CArchMiscWindows::setInstanceWin32(HINSTANCE instance)
{
	assert(instance != NULL);
	s_instanceWin32 = instance;
}