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

//
// CArchMiscWindows
//

bool
CArchMiscWindows::isWindows95Family()
{
	OSVERSIONINFO version;
	version.dwOSVersionInfoSize = sizeof(version);
	if (GetVersionEx(&version) == 0) {
		// cannot determine OS;  assume windows 95 family
		return true;
	}
	return (version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
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

HKEY
CArchMiscWindows::openKey(HKEY key, const TCHAR* keyName)
{
	// ignore if parent is NULL
	if (key == NULL) {
		return NULL;
	}

	// open next key
	HKEY newKey;
	LONG result = RegOpenKeyEx(key, keyName, 0,
								KEY_WRITE | KEY_QUERY_VALUE, &newKey);
	if (result != ERROR_SUCCESS) {
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
CArchMiscWindows::openKey(HKEY key, const TCHAR* const* keyNames)
{
	for (size_t i = 0; key != NULL && keyNames[i] != NULL; ++i) {
		// open next key
		key = openKey(key, keyNames[i]);
	}
	return key;
}

void
CArchMiscWindows::closeKey(HKEY key)
{
	assert(key  != NULL);
	RegCloseKey(key);
}

void
CArchMiscWindows::deleteKey(HKEY key, const TCHAR* name)
{
	assert(key  != NULL);
	assert(name != NULL);
	RegDeleteKey(key, name);
}

void
CArchMiscWindows::deleteValue(HKEY key, const TCHAR* name)
{
	assert(key  != NULL);
	assert(name != NULL);
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

void
CArchMiscWindows::setValue(HKEY key,
				const TCHAR* name, const std::string& value)
{
	assert(key  != NULL);
	assert(name != NULL);
	RegSetValueEx(key, name, 0, REG_SZ,
								reinterpret_cast<const BYTE*>(value.c_str()),
								value.size() + 1);
}

void
CArchMiscWindows::setValue(HKEY key, const TCHAR* name, DWORD value)
{
	assert(key  != NULL);
	assert(name != NULL);
	RegSetValueEx(key, name, 0, REG_DWORD,
								reinterpret_cast<CONST BYTE*>(&value),
								sizeof(DWORD));
}

std::string
CArchMiscWindows::readValueString(HKEY key, const TCHAR* name)
{
	// get the size of the string
	DWORD type;
	DWORD size = 0;
	LONG result = RegQueryValueEx(key, name, 0, &type, NULL, &size);
	if (result != ERROR_SUCCESS || type != REG_SZ) {
		return std::string();
	}

	// allocate space
	char* buffer = new char[size];

	// read it
	result = RegQueryValueEx(key, name, 0, &type,
								reinterpret_cast<BYTE*>(buffer), &size);
	if (result != ERROR_SUCCESS || type != REG_SZ) {
		delete[] buffer;
		return std::string();
	}

	// clean up and return value
	std::string value(buffer);
	delete[] buffer;
	return value;
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
