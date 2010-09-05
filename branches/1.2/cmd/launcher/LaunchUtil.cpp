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

#include "CConfig.h"
#include "LaunchUtil.h"
#include "CMSWindowsUtil.h"
#include "CArch.h"
#include "resource.h"
#include "stdfstream.h"

#define CONFIG_NAME "synergy.sgc"

CString
getString(DWORD id)
{
	return CMSWindowsUtil::getString(s_instance, id);
}

CString
getErrorString(DWORD error)
{
	return CMSWindowsUtil::getErrorString(s_instance, error, IDS_ERROR);
}

void
showError(HWND hwnd, const CString& msg)
{
	CString title = getString(IDS_ERROR);
	MessageBox(hwnd, msg.c_str(), title.c_str(), MB_OK | MB_APPLMODAL);
}

void
askOkay(HWND hwnd, const CString& title, const CString& msg)
{
	MessageBox(hwnd, msg.c_str(), title.c_str(), MB_OK | MB_APPLMODAL);
}

bool
askVerify(HWND hwnd, const CString& msg)
{
	CString title = getString(IDS_VERIFY);
	int result = MessageBox(hwnd, msg.c_str(),
								title.c_str(), MB_OKCANCEL | MB_APPLMODAL);
	return (result == IDOK);
}

void
setWindowText(HWND hwnd, const CString& msg)
{
	SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)msg.c_str());
}

CString
getWindowText(HWND hwnd)
{
	LRESULT size = SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
	char* buffer = new char[size + 1];
	SendMessage(hwnd, WM_GETTEXT, size + 1, (LPARAM)buffer);
	buffer[size] = '\0';
	CString result(buffer);
	delete[] buffer;
	return result;
}

HWND
getItem(HWND hwnd, int id)
{
	return GetDlgItem(hwnd, id);
}

void
enableItem(HWND hwnd, int id, bool enabled)
{
	EnableWindow(GetDlgItem(hwnd, id), enabled);
}

void
setItemChecked(HWND hwnd, bool checked)
{
	SendMessage(hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool
isItemChecked(HWND hwnd)
{
	return (SendMessage(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

CString
getAppPath(const CString& appName)
{
	// prepare path to app
	char myPathname[MAX_PATH];
	GetModuleFileName(s_instance, myPathname, MAX_PATH);
	const char* myBasename = ARCH->getBasename(myPathname);
	CString appPath = CString(myPathname, myBasename - myPathname);
	appPath += appName;
	return appPath;
}

static
bool
loadConfig(const CString& pathname, CConfig& config)
{
	try {
		std::ifstream stream(pathname.c_str());
		if (stream) {
			stream >> config;
			return true;
		}
	}
	catch (...) {
		// ignore
	}
	return false;
}

bool
loadConfig(CConfig& config)
{
	// load configuration
	bool configLoaded = false;
	CString path = ARCH->getUserDirectory();
	if (!path.empty()) {
		// try loading the user's configuration
		path = ARCH->concatPath(path, CONFIG_NAME);
		if (loadConfig(path, config)) {
			configLoaded = true;
		}
		else {
			// try the system-wide config file
			path = ARCH->getSystemDirectory();
			if (!path.empty()) {
				path = ARCH->concatPath(path, CONFIG_NAME);
				if (loadConfig(path, config)) {
					configLoaded = true;
				}
			}
		}
	}
	return configLoaded;
}

static
bool
saveConfig(const CString& pathname, const CConfig& config)
{
	try {
		std::ofstream stream(pathname.c_str());
		if (stream) {
			stream << config;
			return !!stream;
		}
	}
	catch (...) {
		// ignore
	}
	return false;
}

bool
saveConfig(const CConfig& config, bool sysOnly)
{
	// try saving the user's configuration
	if (!sysOnly) {
		CString path = ARCH->getUserDirectory();
		if (!path.empty()) {
			path = ARCH->concatPath(path, CONFIG_NAME);
			if (saveConfig(path, config)) {
				return true;
			}
		}
	}

	// try the system-wide config file
	else {
		CString path = ARCH->getSystemDirectory();
		if (!path.empty()) {
			path = ARCH->concatPath(path, CONFIG_NAME);
			if (saveConfig(path, config)) {
				return true;
			}
		}
	}

	return false;
}

const TCHAR* const*
getSettingsPath()
{
	static const TCHAR* s_keyNames[] = {
		TEXT("Software"),
		TEXT("Synergy"),
		TEXT("Synergy"),
		NULL
	};
	return s_keyNames;
}
