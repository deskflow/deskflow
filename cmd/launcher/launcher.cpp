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
#include "ProtocolTypes.h"
#include "CPlatform.h"
#include "CNetwork.h"
#include "Version.h"
#include "stdfstream.h"
#include "stdvector.h"
#include "resource.h"

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#define CONFIG_NAME "synergy.sgc"
#define CLIENT_APP "synergyc.exe"
#define SERVER_APP "synergyd.exe"

typedef std::vector<CString> CStringList;

class CScreenInfo {
public:
	CString				m_screen;
	CStringList			m_aliases;
};

class CChildWaitInfo {
public:
	HWND				m_dialog;
	HANDLE				m_child;
	DWORD				m_childID;
	HANDLE				m_ready;
	HANDLE				m_stop;
};

static const TCHAR* s_mainClass   = TEXT("GoSynergy");
static const TCHAR* s_layoutClass = TEXT("SynergyLayout");
static HINSTANCE s_instance       = NULL;

static HWND s_mainWindow;
static CConfig s_config;
static CConfig s_oldConfig;
static CStringList s_screens;

static
BOOL CALLBACK
addDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static
void
tokenize(CStringList& tokens, const CString& src)
{
	// find first non-whitespace
	CString::size_type x = src.find_first_not_of(" \t\r\n");
	if (x == CString::npos) {
		return;
	}

	// find next whitespace
	do {
		CString::size_type y = src.find_first_of(" \t\r\n", x);
		if (y == CString::npos) {
			y = src.size();
		}
		tokens.push_back(src.substr(x, y - x));
		x = src.find_first_not_of(" \t\r\n", y);
	} while (x != CString::npos);
}

static
bool
isNameInList(const CStringList& names, const CString& name)
{
	for (CStringList::const_iterator index = names.begin();
								index != names.end(); ++index) {
		if (CStringUtil::CaselessCmp::equal(name, *index)) {
			return true;
		}
	}
	return false;
}

static
CString
getString(DWORD id)
{
	char buffer[1024];
	buffer[0] = '\0';
	LoadString(s_instance, id, buffer, sizeof(buffer) / sizeof(buffer[0]));
	return buffer;
}

static
CString
getErrorString(DWORD error)
{
	char* buffer;
	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
								FORMAT_MESSAGE_IGNORE_INSERTS |
								FORMAT_MESSAGE_FROM_SYSTEM,
								0,
								error,
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
								(LPTSTR)&buffer,
								0,
								NULL) == 0) {
		return getString(IDS_ERROR);
	}
	else {
		CString result(buffer);
		LocalFree(buffer);
		return result;
	}
}

static
void
showError(HWND hwnd, const CString& msg)
{
	CString title = getString(IDS_ERROR);
	MessageBox(hwnd, msg.c_str(), title.c_str(), MB_OK | MB_APPLMODAL);
}

static
void
askOkay(HWND hwnd, const CString& title, const CString& msg)
{
	MessageBox(hwnd, msg.c_str(), title.c_str(), MB_OK | MB_APPLMODAL);
}

static
bool
askVerify(HWND hwnd, const CString& msg)
{
	CString title = getString(IDS_VERIFY);
	int result = MessageBox(hwnd, msg.c_str(),
								title.c_str(), MB_OKCANCEL | MB_APPLMODAL);
	return (result == IDOK);
}

static
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

static
void
enableItem(HWND hwnd, int id, bool enabled)
{
	EnableWindow(GetDlgItem(hwnd, id), enabled);
}

static
bool
isClientChecked(HWND hwnd)
{
	HWND child = GetDlgItem(hwnd, IDC_MAIN_CLIENT_RADIO);
	return (SendMessage(child, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

static
void
enableScreensControls(HWND hwnd)
{
	// decide if edit and remove buttons should be enabled
	bool client         = isClientChecked(hwnd);
	bool screenSelected = false;
	if (!client) {
		HWND child = GetDlgItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST);
		if (SendMessage(child, LB_GETCURSEL, 0, 0) != LB_ERR) {
			screenSelected = true;
		}
	}

	// enable/disable controls
	enableItem(hwnd, IDC_MAIN_SERVER_SCREENS_LABEL, !client);
	enableItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST, !client);
	enableItem(hwnd, IDC_MAIN_SERVER_ADD_BUTTON, !client);
	enableItem(hwnd, IDC_MAIN_SERVER_EDIT_BUTTON, screenSelected);
	enableItem(hwnd, IDC_MAIN_SERVER_REMOVE_BUTTON, screenSelected);
	enableItem(hwnd, IDC_MAIN_SERVER_LAYOUT_LABEL, !client);
	enableItem(hwnd, IDC_MAIN_SERVER_LEFT_COMBO, screenSelected);
	enableItem(hwnd, IDC_MAIN_SERVER_RIGHT_COMBO, screenSelected);
	enableItem(hwnd, IDC_MAIN_SERVER_TOP_COMBO, screenSelected);
	enableItem(hwnd, IDC_MAIN_SERVER_BOTTOM_COMBO, screenSelected);
	enableItem(hwnd, IDC_MAIN_SERVER_LEFT_LABEL, screenSelected);
	enableItem(hwnd, IDC_MAIN_SERVER_RIGHT_LABEL, screenSelected);
	enableItem(hwnd, IDC_MAIN_SERVER_TOP_LABEL, screenSelected);
	enableItem(hwnd, IDC_MAIN_SERVER_BOTTOM_LABEL, screenSelected);
}

static
void
enableMainWindowControls(HWND hwnd)
{
	bool client = isClientChecked(hwnd);
	enableItem(hwnd, IDC_MAIN_CLIENT_SERVER_NAME_LABEL, client);
	enableItem(hwnd, IDC_MAIN_CLIENT_SERVER_NAME_EDIT, client);
	enableScreensControls(hwnd);
}

static
void
updateNeighbor(HWND hwnd, const CString& screen, EDirection direction)
{
	// remove all neighbors from combo box
	SendMessage(hwnd, CB_RESETCONTENT, 0, 0);

	// add all screens to combo box
	if (!screen.empty()) {
		for (CConfig::const_iterator index = s_config.begin();
								index != s_config.end(); ++index) {
			SendMessage(hwnd, CB_INSERTSTRING,
								(WPARAM)-1, (LPARAM)index->c_str());
		}
	}

	// add empty neighbor to combo box
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"---");

	// select neighbor in combo box
	LRESULT index = 0;
	if (!screen.empty()) {
		const CString& neighbor = s_config.getNeighbor(screen, direction);
		if (!neighbor.empty()) {
			index = SendMessage(hwnd, CB_FINDSTRINGEXACT,
								0, (LPARAM)neighbor.c_str());
			if (index == LB_ERR) {
				index = 0;
			}
		}
	}
	SendMessage(hwnd, CB_SETCURSEL, index, 0);
}

static
void
updateNeighbors(HWND hwnd)
{
	// get selected screen name or empty string if no selection
	CString screen;
	HWND child    = GetDlgItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST);
	LRESULT index = SendMessage(child, LB_GETCURSEL, 0, 0);
	if (index != LB_ERR) {
		screen = s_screens[index];
	}

	// set neighbor combo boxes
	child = GetDlgItem(hwnd, IDC_MAIN_SERVER_LEFT_COMBO);
	updateNeighbor(child, screen, kLeft);
	child = GetDlgItem(hwnd, IDC_MAIN_SERVER_RIGHT_COMBO);
	updateNeighbor(child, screen, kRight);
	child = GetDlgItem(hwnd, IDC_MAIN_SERVER_TOP_COMBO);
	updateNeighbor(child, screen, kTop);
	child = GetDlgItem(hwnd, IDC_MAIN_SERVER_BOTTOM_COMBO);
	updateNeighbor(child, screen, kBottom);
}

static
void
addScreen(HWND hwnd)
{
	// empty screen info
	CScreenInfo info;

	// run dialog
	if (DialogBoxParam(s_instance, MAKEINTRESOURCE(IDD_ADD),
								hwnd, addDlgProc, (LPARAM)&info) != 0) {
		// get current number of screens
		UInt32 i = s_screens.size();

		// add screen to list control
		HWND child = GetDlgItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST);
		CString item = CStringUtil::print("%d. %s",
								i + 1, info.m_screen.c_str());
		SendMessage(child, LB_ADDSTRING, 0, (LPARAM)item.c_str());

		// add screen to screen list
		s_screens.push_back(info.m_screen);

		// add screen to config
		s_config.addScreen(info.m_screen);

		// add aliases to config
		for (CStringList::const_iterator index = info.m_aliases.begin();
								index != info.m_aliases.end(); ++index) {
			s_config.addAlias(info.m_screen, *index);
		}

		// update neighbors
		updateNeighbors(hwnd);
		enableScreensControls(hwnd);
	}
}

static
void
editScreen(HWND hwnd)
{
	// get selected list item
	HWND child    = GetDlgItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST);
	LRESULT index = SendMessage(child, LB_GETCURSEL, 0, 0);
	if (index == LB_ERR) {
		// no selection
		return;
	}

	// fill in screen info
	CScreenInfo info;
	info.m_screen = s_screens[index];
	for (CConfig::all_const_iterator index = s_config.beginAll();
								index != s_config.endAll(); ++index) {
		if (CStringUtil::CaselessCmp::equal(index->second, info.m_screen) &&
			!CStringUtil::CaselessCmp::equal(index->second, index->first)) {
			info.m_aliases.push_back(index->first);
		}
	}

	// save current info
	CScreenInfo oldInfo = info;

	// run dialog
	if (DialogBoxParam(s_instance, MAKEINTRESOURCE(IDD_ADD),
								hwnd, addDlgProc, (LPARAM)&info) != 0) {
		// replace screen
		s_screens[index] = info.m_screen;

		// remove old aliases
		for (CStringList::const_iterator index = oldInfo.m_aliases.begin();
								index != oldInfo.m_aliases.end(); ++index) {
			s_config.removeAlias(*index);
		}

		// replace name
		s_config.renameScreen(oldInfo.m_screen, info.m_screen);

		// add new aliases
		for (CStringList::const_iterator index = info.m_aliases.begin();
								index != info.m_aliases.end(); ++index) {
			s_config.addAlias(info.m_screen, *index);
		}

		// update list
		CString item = CStringUtil::print("%d. %s",
								index + 1, info.m_screen.c_str());
		SendMessage(child, LB_DELETESTRING, index, 0);
		SendMessage(child, LB_INSERTSTRING, index,
								(LPARAM)item.c_str());
		SendMessage(child, LB_SETCURSEL, index, 0);

		// update neighbors
		updateNeighbors(hwnd);
	}
}

static
void
removeScreen(HWND hwnd)
{
	// get selected list item
	HWND child    = GetDlgItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST);
	LRESULT index = SendMessage(child, LB_GETCURSEL, 0, 0);
	if (index == LB_ERR) {
		// no selection
		return;
	}

	// get screen name
	CString name = s_screens[index];

	// remove screen from list control
	SendMessage(child, LB_DELETESTRING, index, 0);

	// remove screen from screen list
	s_screens.erase(&s_screens[index]);

	// remove screen from config (this also removes aliases)
	s_config.removeScreen(name);

	// update neighbors
	updateNeighbors(hwnd);
	enableScreensControls(hwnd);
}

static
void
changeNeighbor(HWND hwnd, HWND combo, EDirection direction)
{
	// get selected screen
	HWND child    = GetDlgItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST);
	LRESULT index = SendMessage(child, LB_GETCURSEL, 0, 0);
	if (index == LB_ERR) {
		// no selection
		return;
	}

	// get screen name
	CString screen = s_screens[index];

	// get selected neighbor
	index = SendMessage(combo, CB_GETCURSEL, 0, 0);

	// remove old connection
	s_config.disconnect(screen, direction);

	// add new connection
	if (index != LB_ERR && index != 0) {
		LRESULT size = SendMessage(combo, CB_GETLBTEXTLEN, index, 0);
		char* neighbor = new char[size + 1];
		SendMessage(combo, CB_GETLBTEXT, index, (LPARAM)neighbor);
		s_config.connect(screen, direction, CString(neighbor));
		delete[] neighbor;
	}
}

static
bool
execApp(const char* app, const CString& cmdLine, PROCESS_INFORMATION* procInfo)
{
	// prepare startup info
	STARTUPINFO startup;
	startup.cb              = sizeof(startup);
	startup.lpReserved      = NULL;
	startup.lpDesktop       = NULL;
	startup.lpTitle         = NULL;
	startup.dwX             = (DWORD)CW_USEDEFAULT;
	startup.dwY             = (DWORD)CW_USEDEFAULT;
	startup.dwXSize         = (DWORD)CW_USEDEFAULT;
	startup.dwYSize         = (DWORD)CW_USEDEFAULT;
	startup.dwXCountChars   = 0;
	startup.dwYCountChars   = 0;
	startup.dwFillAttribute = 0;
	startup.dwFlags         = STARTF_FORCEONFEEDBACK;
	startup.wShowWindow     = SW_SHOWDEFAULT;
	startup.cbReserved2     = 0;
	startup.lpReserved2     = NULL;
	startup.hStdInput       = NULL;
	startup.hStdOutput      = NULL;
	startup.hStdError       = NULL;

	// prepare path to app
	CPlatform platform;
	char myPathname[MAX_PATH];
	GetModuleFileName(s_instance, myPathname, MAX_PATH);
	const char* myBasename = platform.getBasename(myPathname);
	CString appPath = CString(myPathname, myBasename - myPathname);
	appPath += app;

	// start child
	if (CreateProcess(appPath.c_str(),
								(char*)cmdLine.c_str(),
								NULL,
								NULL,
								FALSE,
								CREATE_DEFAULT_ERROR_MODE |
									CREATE_NEW_PROCESS_GROUP |
									NORMAL_PRIORITY_CLASS,
								NULL,
								NULL,
								&startup,
								procInfo) == 0) {
		return false;
	}
	else {
		return true;
	}
}

static
bool
uninstallApp(const char* app)
{
	PROCESS_INFORMATION procInfo;

	// uninstall
	DWORD exitCode = kExitFailed;
	if (execApp(app, "-z --uninstall", &procInfo)) {
		WaitForSingleObject(procInfo.hProcess, INFINITE);
		GetExitCodeProcess(procInfo.hProcess, &exitCode);
		CloseHandle(procInfo.hProcess);
		CloseHandle(procInfo.hThread);
	}

	return (exitCode == kExitSuccess);
}

static
HANDLE
launchApp(HWND hwnd, bool testing, DWORD* threadID)
{
	// decide if client or server
	const bool isClient = isClientChecked(hwnd);
	const char* app = isClient ? CLIENT_APP : SERVER_APP;

	// get and verify screen name
	HWND child = GetDlgItem(hwnd, IDC_MAIN_ADVANCED_NAME_EDIT);
	CString name = getWindowText(child);
	if (!s_config.isValidScreenName(name)) {
		showError(hwnd, CStringUtil::format(
								getString(IDS_INVALID_SCREEN_NAME).c_str(),
								name.c_str()));
		SetFocus(child);
		return NULL;
	}
	if (!isClient && !s_config.isScreen(name)) {
		showError(hwnd, CStringUtil::format(
								getString(IDS_UNKNOWN_SCREEN_NAME).c_str(),
								name.c_str()));
		SetFocus(child);
		return NULL;
	}

	// get and verify port
	child = GetDlgItem(hwnd, IDC_MAIN_ADVANCED_PORT_EDIT);
	CString portString = getWindowText(child);
	UInt32 port = (UInt32)atoi(portString.c_str());
	if (port < 1 || port > 65535) {
		CString defaultPortString = CStringUtil::print("%d", kDefaultPort);
		showError(hwnd, CStringUtil::format(
								getString(IDS_INVALID_PORT).c_str(),
								portString.c_str(),
								defaultPortString.c_str()));
		SetFocus(child);
		return NULL;
	}

	// prepare command line
	CString cmdLine;
	if (testing) {
		cmdLine += " -z --no-restart --no-daemon";
	}
	cmdLine += " --name ";
	cmdLine += name;
	if (isClient) {
		// check server name
		child = GetDlgItem(hwnd, IDC_MAIN_CLIENT_SERVER_NAME_EDIT);
		CString server = getWindowText(child);
		if (!s_config.isValidScreenName(server)) {
			showError(hwnd, CStringUtil::format(
								getString(IDS_INVALID_SCREEN_NAME).c_str(),
								server.c_str()));
			SetFocus(child);
			return NULL;
		}

		if (testing) {
			cmdLine += " --no-camp";
		}
		cmdLine += " ";
		cmdLine += server;
		cmdLine += ":";
		cmdLine += portString;
	}
	else {
		cmdLine += " --address :";
		cmdLine += portString;
	}

	// uninstall client and server then reinstall one of them
	if (!testing) {
		// uninstall client and server
		uninstallApp(CLIENT_APP);
		uninstallApp(SERVER_APP);

		// install client or server
		PROCESS_INFORMATION procInfo;
		DWORD exitCode = kExitFailed;
		if (execApp(app, CString("-z --install") + cmdLine, &procInfo)) {
			WaitForSingleObject(procInfo.hProcess, INFINITE);
			GetExitCodeProcess(procInfo.hProcess, &exitCode);
			CloseHandle(procInfo.hProcess);
			CloseHandle(procInfo.hThread);
		}

		// see if install succeeded
		if (exitCode != kExitSuccess) {
			showError(hwnd, getString(IDS_INSTALL_FAILED).c_str());
			return NULL;
		}
	}

	// start child
	PROCESS_INFORMATION procInfo;
	if (!execApp(app, cmdLine, &procInfo)) {
		showError(hwnd, CStringUtil::format(
								getString(IDS_STARTUP_FAILED).c_str(),
								getErrorString(GetLastError()).c_str()));
		return NULL;
	}

	// don't need process handle
	CloseHandle(procInfo.hProcess);

	// save thread ID if desired
	if (threadID != NULL) {
		*threadID = procInfo.dwThreadId;
	}

	// return thread handle
	return procInfo.hThread;
}

static
BOOL CALLBACK
waitDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// only one wait dialog at a time!
	static CChildWaitInfo* info = NULL;

	switch (message) {
	case WM_INITDIALOG:
		// save info pointer
		info = reinterpret_cast<CChildWaitInfo*>(lParam);

		// save hwnd
		info->m_dialog = hwnd;

		// signal ready
		SetEvent(info->m_ready);

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
		case IDOK:
			// signal stop
			SetEvent(info->m_stop);

			// done
			EndDialog(hwnd, 0);
			return TRUE;
		}
	}

	return FALSE;
}

static
DWORD WINAPI
waitForChildThread(LPVOID vinfo)
{
	CChildWaitInfo* info = reinterpret_cast<CChildWaitInfo*>(vinfo);

	// wait for ready
	WaitForSingleObject(info->m_ready, INFINITE);

	// wait for thread to complete or stop event
	HANDLE handles[2];
	handles[0] = info->m_child;
	handles[1] = info->m_stop;
	DWORD n = WaitForMultipleObjects(2, handles, FALSE, INFINITE);

	// if stop was raised then terminate child and wait for it
	if (n == WAIT_OBJECT_0 + 1) {
		PostThreadMessage(info->m_childID, WM_QUIT, 0, 0);
		WaitForSingleObject(info->m_child, INFINITE);
	}

	// otherwise post IDOK to dialog box
	else {
		PostMessage(info->m_dialog, WM_COMMAND, MAKEWPARAM(IDOK, 0), 0);
	}

	return 0;
}

static
void
waitForChild(HWND hwnd, HANDLE thread, DWORD threadID)
{
	// prepare info for child wait dialog and thread
	CChildWaitInfo info;
	info.m_dialog  = NULL;
	info.m_child   = thread;
	info.m_childID = threadID;
	info.m_ready   = CreateEvent(NULL, TRUE, FALSE, NULL);
	info.m_stop    = CreateEvent(NULL, TRUE, FALSE, NULL);

	// create a thread to wait on the child thread and event
	DWORD id;
	HANDLE waiter = CreateThread(NULL, 0, &waitForChildThread, &info,0, &id);

	// do dialog that let's the user terminate the test
	DialogBoxParam(s_instance, MAKEINTRESOURCE(IDD_WAIT), hwnd,
								waitDlgProc, (LPARAM)&info);

	// force the waiter thread to finish and wait for it
	SetEvent(info.m_ready);
	SetEvent(info.m_stop);
	WaitForSingleObject(waiter, INFINITE);

	// clean up
	CloseHandle(waiter);
	CloseHandle(info.m_ready);
	CloseHandle(info.m_stop);
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

static
bool
saveConfig(const CConfig& config)
{
	CPlatform platform;

	CString path = platform.getUserDirectory();
	if (!path.empty()) {
		// try loading the user's configuration
		path = platform.addPathComponent(path, CONFIG_NAME);
		if (saveConfig(path, config)) {
			return true;
		}
	}

	// try the system-wide config file
	path = platform.getSystemDirectory();
	if (!path.empty()) {
		path = platform.addPathComponent(path, CONFIG_NAME);
		if (saveConfig(path, config)) {
			return true;
		}
	}

	return false;
}

static
void
initMainWindow(HWND hwnd)
{
	CPlatform platform;

	// load configuration
	bool configLoaded = false;
	CString path = platform.getUserDirectory();
	if (!path.empty()) {
		// try loading the user's configuration
		path = platform.addPathComponent(path, CONFIG_NAME);
		if (loadConfig(path, s_config)) {
			configLoaded = true;
		}
		else {
			// try the system-wide config file
			path = platform.getSystemDirectory();
			if (!path.empty()) {
				path = platform.addPathComponent(path, CONFIG_NAME);
				if (loadConfig(path, s_config)) {
					configLoaded = true;
				}
			}
		}
	}
	s_oldConfig = s_config;

	// choose client/server radio buttons
	HWND child;
	child = GetDlgItem(hwnd, IDC_MAIN_CLIENT_RADIO);
	SendMessage(child, BM_SETCHECK, !configLoaded ?
								BST_CHECKED : BST_UNCHECKED, 0);
	child = GetDlgItem(hwnd, IDC_MAIN_SERVER_RADIO);
	SendMessage(child, BM_SETCHECK, configLoaded ?
								BST_CHECKED : BST_UNCHECKED, 0);

	// if config is loaded then initialize server controls
	if (configLoaded) {
		int i = 1;
		child = GetDlgItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST);
		for (CConfig::const_iterator index = s_config.begin();
								index != s_config.end(); ++i, ++index) {
			s_screens.push_back(*index);
			CString item = CStringUtil::print("%d. %s", i, index->c_str());
			SendMessage(child, LB_ADDSTRING, 0, (LPARAM)item.c_str());
		}
	}

	// initialize other controls
	char buffer[256];
	sprintf(buffer, "%d", kDefaultPort);
	child = GetDlgItem(hwnd, IDC_MAIN_ADVANCED_PORT_EDIT);
	SendMessage(child, WM_SETTEXT, 0, (LPARAM)buffer);

	CNetwork::gethostname(buffer, sizeof(buffer));
	child = GetDlgItem(hwnd, IDC_MAIN_ADVANCED_NAME_EDIT);
	SendMessage(child, WM_SETTEXT, 0, (LPARAM)buffer);

	// update neighbor combo boxes
	enableMainWindowControls(hwnd);
	updateNeighbors(hwnd);
}

static
BOOL CALLBACK
addDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// only one add dialog at a time!
	static CScreenInfo* info = NULL;

	switch (message) {
	case WM_INITDIALOG: {
		info = (CScreenInfo*)lParam;

		// fill in screen name
		HWND child = GetDlgItem(hwnd, IDC_ADD_SCREEN_NAME_EDIT);
		SendMessage(child, WM_SETTEXT, 0, (LPARAM)info->m_screen.c_str());

		// fill in aliases
		CString aliases;
		for (CStringList::const_iterator index = info->m_aliases.begin();
								index != info->m_aliases.end(); ++index) {
			if (!aliases.empty()) {
				aliases += "\r\n";
			}
			aliases += *index;
		}
		child = GetDlgItem(hwnd, IDC_ADD_ALIASES_EDIT);
		SendMessage(child, WM_SETTEXT, 0, (LPARAM)aliases.c_str());

		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: {
			CString newName;
			CStringList newAliases;

			// extract name and aliases
			HWND child = GetDlgItem(hwnd, IDC_ADD_SCREEN_NAME_EDIT);
			newName = getWindowText(child);
			child = GetDlgItem(hwnd, IDC_ADD_ALIASES_EDIT);
			tokenize(newAliases, getWindowText(child));

			// name must be valid
			if (!s_config.isValidScreenName(newName)) {
				showError(hwnd, CStringUtil::format(
								getString(IDS_INVALID_SCREEN_NAME).c_str(),
								newName.c_str()));
				return TRUE;
			}

			// aliases must be valid
			for (CStringList::const_iterator index = newAliases.begin();
								index != newAliases.end(); ++index) {
				if (!s_config.isValidScreenName(*index)) {
					showError(hwnd, CStringUtil::format(
								getString(IDS_INVALID_SCREEN_NAME).c_str(),
								index->c_str()));
					return TRUE;
				}
			}

			// new name may not be in the new alias list
			if (isNameInList(newAliases, newName)) {
				showError(hwnd, CStringUtil::format(
								getString(IDS_SCREEN_NAME_IS_ALIAS).c_str(),
								newName.c_str()));
				return TRUE;
			}

			// name must not exist in config but allow same name.  also
			// allow name if it exists in the old alias list but not the
			// new one.
			if (s_config.isScreen(newName) &&
				!CStringUtil::CaselessCmp::equal(newName, info->m_screen) &&
				!isNameInList(info->m_aliases, newName)) {
				showError(hwnd, CStringUtil::format(
								getString(IDS_DUPLICATE_SCREEN_NAME).c_str(),
								newName.c_str()));
				return TRUE;
			}

			// aliases must not exist in config but allow same aliases and
			// allow an alias to be the old name.
			for (CStringList::const_iterator index = newAliases.begin();
								index != newAliases.end(); ++index) {
				if (s_config.isScreen(*index) &&
					!CStringUtil::CaselessCmp::equal(*index, info->m_screen) &&
					!isNameInList(info->m_aliases, *index)) {
					showError(hwnd, CStringUtil::format(
								getString(IDS_DUPLICATE_SCREEN_NAME).c_str(),
								index->c_str()));
					return TRUE;
				}
			}

			// save data
			info->m_screen  = newName;
			info->m_aliases = newAliases;

			// success
			EndDialog(hwnd, 1);
			info = NULL;
			return TRUE;
		}

		case IDCANCEL:
			EndDialog(hwnd, 0);
			info = NULL;
			return TRUE;
		}

	default:
		break;
	}

	return FALSE;
}

static
LRESULT CALLBACK
mainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			// test for unsaved data
			if (s_config != s_oldConfig) {
				if (!askVerify(hwnd, getString(IDS_UNSAVED_DATA_REALLY_QUIT))) {
					return 0;
				}
			}

			// quit
			PostQuitMessage(0);
			return 0;

		case IDOK: {
			// save data
			if (s_config != s_oldConfig) {
				if (!saveConfig(s_config)) {
					showError(hwnd, CStringUtil::format(
								getString(IDS_SAVE_FAILED).c_str(),
								getErrorString(GetLastError()).c_str()));
					return 0;
				}
				s_oldConfig = s_config;
			}

			// launch child app
			HANDLE thread = launchApp(hwnd, false, NULL);
			if (thread == NULL) {
				return 0;
			}
			CloseHandle(thread);

			// notify of success
			askOkay(hwnd, getString(IDS_STARTED_TITLE),
								getString(IDS_STARTED));

			// quit
			PostQuitMessage(0);
			return 0;
		}

		case IDC_MAIN_TEST: {
			// save data
			if (s_config != s_oldConfig) {
				if (!saveConfig(s_config)) {
					showError(hwnd, CStringUtil::format(
								getString(IDS_SAVE_FAILED).c_str(),
								getErrorString(GetLastError()).c_str()));
					return 0;
				}
				s_oldConfig = s_config;
			}

			// launch child app
			DWORD threadID;
			HANDLE thread = launchApp(hwnd, true, &threadID);
			if (thread == NULL) {
				return 0;
			}

			// wait for process to stop, allowing the user to kill it
			waitForChild(hwnd, thread, threadID);

			// clean up
			CloseHandle(thread);
			return 0;
		}

		case IDC_MAIN_UNINSTALL: {
			// uninstall client and server
			bool removedClient = uninstallApp(CLIENT_APP);
			bool removedServer = uninstallApp(SERVER_APP);
			if (!removedClient) {
				showError(hwnd, CStringUtil::format(
								getString(IDS_UNINSTALL_FAILED).c_str(),
								getString(IDS_CLIENT).c_str()));
			}
			else if (!removedServer) {
				showError(hwnd, CStringUtil::format(
								getString(IDS_UNINSTALL_FAILED).c_str(),
								getString(IDS_SERVER).c_str()));
			}
			else {
				askOkay(hwnd, getString(IDS_UNINSTALL_TITLE),
								getString(IDS_UNINSTALLED));
			}
			return 0;
		}

		case IDC_MAIN_CLIENT_RADIO:
		case IDC_MAIN_SERVER_RADIO:
			enableMainWindowControls(hwnd);
			return 0;

		case IDC_MAIN_SERVER_ADD_BUTTON:
			addScreen(hwnd);
			return 0;

		case IDC_MAIN_SERVER_EDIT_BUTTON:
			editScreen(hwnd);
			return 0;

		case IDC_MAIN_SERVER_REMOVE_BUTTON:
			removeScreen(hwnd);
			return 0;

		case IDC_MAIN_SERVER_SCREENS_LIST:
			if (HIWORD(wParam) == LBN_SELCHANGE) {
				enableScreensControls(hwnd);
				updateNeighbors(hwnd);
			}
			else if (HIWORD(wParam) == LBN_DBLCLK) {
				editScreen(hwnd);
				return 0;
			}
			break;

		case IDC_MAIN_SERVER_LEFT_COMBO:
			if (HIWORD(wParam) == CBN_SELENDOK) {
				changeNeighbor(hwnd, (HWND)lParam, kLeft);
				return 0;
			}
			break;

		case IDC_MAIN_SERVER_RIGHT_COMBO:
			if (HIWORD(wParam) == CBN_SELENDOK) {
				changeNeighbor(hwnd, (HWND)lParam, kRight);
				return 0;
			}
			break;

		case IDC_MAIN_SERVER_TOP_COMBO:
			if (HIWORD(wParam) == CBN_SELENDOK) {
				changeNeighbor(hwnd, (HWND)lParam, kTop);
				return 0;
			}
			break;

		case IDC_MAIN_SERVER_BOTTOM_COMBO:
			if (HIWORD(wParam) == CBN_SELENDOK) {
				changeNeighbor(hwnd, (HWND)lParam, kBottom);
				return 0;
			}
			break;
		}

	default:
		break;
	}
	return DefDlgProc(hwnd, message, wParam, lParam);
}

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int nCmdShow)
{
	s_instance = instance;

	// initialize network library
	CNetwork::init();

	// register main window (dialog) class
	WNDCLASSEX classInfo;
	classInfo.cbSize        = sizeof(classInfo);
	classInfo.style         = CS_HREDRAW | CS_VREDRAW;
	classInfo.lpfnWndProc   = &mainWndProc;
	classInfo.cbClsExtra    = 0;
	classInfo.cbWndExtra    = DLGWINDOWEXTRA;
	classInfo.hInstance     = instance;
	classInfo.hIcon         = (HICON)LoadImage(instance,
									MAKEINTRESOURCE(IDI_SYNERGY),
									IMAGE_ICON,
									32, 32, LR_SHARED);
	classInfo.hCursor       = LoadCursor(NULL, IDC_ARROW);
	classInfo.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
	classInfo.lpszMenuName  = NULL;
	classInfo.lpszClassName = s_mainClass;
	classInfo.hIconSm       = (HICON)LoadImage(instance,
									MAKEINTRESOURCE(IDI_SYNERGY),
									IMAGE_ICON,
									16, 16, LR_SHARED);
	RegisterClassEx(&classInfo);

	// create main window
	s_mainWindow = CreateDialog(s_instance, MAKEINTRESOURCE(IDD_MAIN), 0, NULL);

	// prep window
	initMainWindow(s_mainWindow);

	// show window
	ShowWindow(s_mainWindow, nCmdShow);

	// main loop
	MSG msg;
	bool done = false;
	do {
		switch (GetMessage(&msg, NULL, 0, 0)) {
		case -1:
			// error
			break;

		case 0:
			// quit
			done = true;
			break;

		default:
			if (!IsDialogMessage(s_mainWindow, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			break;
		}
	} while (!done);

	return msg.wParam;
}
