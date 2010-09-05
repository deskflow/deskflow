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
#include "KeyTypes.h"
#include "OptionTypes.h"
#include "ProtocolTypes.h"
#include "CLog.h"
#include "CStringUtil.h"
#include "CArch.h"
#include "CArchMiscWindows.h"
#include "Version.h"
#include "stdvector.h"
#include "resource.h"

// these must come after the above because it includes windows.h
#include "LaunchUtil.h"
#include "CAutoStart.h"
#include "CGlobalOptions.h"
#include "CAdvancedOptions.h"

#define CONFIG_NAME "synergy.sgc"
#define CLIENT_APP "synergyc.exe"
#define SERVER_APP "synergys.exe"

typedef std::vector<CString> CStringList;

class CScreenInfo {
public:
	CString					m_screen;
	CStringList				m_aliases;
	CConfig::CScreenOptions	m_options;
};

class CChildWaitInfo {
public:
	HWND				m_dialog;
	HANDLE				m_child;
	DWORD				m_childID;
	HANDLE				m_ready;
	HANDLE				m_stop;
};

struct CModifierInfo {
public:
	int				m_ctrlID;
	const char*		m_name;
	KeyModifierID	m_modifierID;
	OptionID		m_optionID;
};

static const CModifierInfo s_modifiers[] = {
	{ IDC_ADD_MOD_SHIFT, "Shift",
		kKeyModifierIDShift,    kOptionModifierMapForShift   },
	{ IDC_ADD_MOD_CTRL,  "Ctrl",
		kKeyModifierIDControl,  kOptionModifierMapForControl },
	{ IDC_ADD_MOD_ALT,   "Alt",
		kKeyModifierIDAlt,      kOptionModifierMapForAlt     },
	{ IDC_ADD_MOD_META,  "Meta",
		kKeyModifierIDMeta,     kOptionModifierMapForMeta    },
	{ IDC_ADD_MOD_SUPER, "Super",
		kKeyModifierIDSuper,    kOptionModifierMapForSuper   }
};

static const KeyModifierID baseModifier = kKeyModifierIDShift;

static const char* s_debugName[][2] = {
	{ TEXT("Error"),   "ERROR" },
	{ TEXT("Warning"), "WARNING" },
	{ TEXT("Note"),    "NOTE" },
	{ TEXT("Info"),    "INFO" },
	{ TEXT("Debug"),   "DEBUG" },
	{ TEXT("Debug1"),  "DEBUG1" },
	{ TEXT("Debug2"),  "DEBUG2" }
};
static const int s_defaultDebug = 1;	// WARNING
static const int s_minTestDebug = 3;	// INFO

HINSTANCE s_instance = NULL;

static CGlobalOptions*		s_globalOptions   = NULL;
static CAdvancedOptions*	s_advancedOptions = NULL;

static const TCHAR* s_mainClass   = TEXT("GoSynergy");
static const TCHAR* s_layoutClass = TEXT("SynergyLayout");

//
// program arguments
//

#define ARG CArgs::s_instance

class CArgs {
public:
	CArgs() { s_instance = this; }
	~CArgs() { s_instance = NULL; }

public:
	static CArgs*		s_instance;
	CConfig				m_config;
	CConfig				m_oldConfig;
	CStringList			m_screens;
};

CArgs*					CArgs::s_instance = NULL;


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
bool
isClientChecked(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_MAIN_CLIENT_RADIO);
	return isItemChecked(child);
}

static
void
enableSaveControls(HWND hwnd)
{
	enableItem(hwnd, IDC_MAIN_SAVE, ARG->m_config != ARG->m_oldConfig);
}

static
void
enableScreensControls(HWND hwnd)
{
	// decide if edit and remove buttons should be enabled
	bool client         = isClientChecked(hwnd);
	bool screenSelected = false;
	if (!client) {
		HWND child = getItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST);
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
	enableSaveControls(hwnd);
}

static
void
updateNeighbor(HWND hwnd, const CString& screen, EDirection direction)
{
	// remove all neighbors from combo box
	SendMessage(hwnd, CB_RESETCONTENT, 0, 0);

	// add all screens to combo box
	if (!screen.empty()) {
		for (CConfig::const_iterator index  = ARG->m_config.begin();
									 index != ARG->m_config.end(); ++index) {
			SendMessage(hwnd, CB_INSERTSTRING,
								(WPARAM)-1, (LPARAM)index->c_str());
		}
	}

	// add empty neighbor to combo box
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)TEXT("---"));

	// select neighbor in combo box
	LRESULT index = 0;
	if (!screen.empty()) {
		const CString& neighbor = ARG->m_config.getNeighbor(screen, direction);
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
	HWND child    = getItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST);
	LRESULT index = SendMessage(child, LB_GETCURSEL, 0, 0);
	if (index != LB_ERR) {
		screen = ARG->m_screens[index];
	}

	// set neighbor combo boxes
	child = getItem(hwnd, IDC_MAIN_SERVER_LEFT_COMBO);
	updateNeighbor(child, screen, kLeft);
	child = getItem(hwnd, IDC_MAIN_SERVER_RIGHT_COMBO);
	updateNeighbor(child, screen, kRight);
	child = getItem(hwnd, IDC_MAIN_SERVER_TOP_COMBO);
	updateNeighbor(child, screen, kTop);
	child = getItem(hwnd, IDC_MAIN_SERVER_BOTTOM_COMBO);
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
		UInt32 i = ARG->m_screens.size();

		// add screen to list control
		HWND child = getItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST);
		CString item = CStringUtil::print("%d. %s",
								i + 1, info.m_screen.c_str());
		SendMessage(child, LB_ADDSTRING, 0, (LPARAM)item.c_str());

		// add screen to screen list
		ARG->m_screens.push_back(info.m_screen);

		// add screen to config
		ARG->m_config.addScreen(info.m_screen);

		// add aliases to config
		for (CStringList::const_iterator index = info.m_aliases.begin();
								index != info.m_aliases.end(); ++index) {
			ARG->m_config.addAlias(info.m_screen, *index);
		}

		// set options
		ARG->m_config.removeOptions(info.m_screen);
		for (CConfig::CScreenOptions::const_iterator
								index  = info.m_options.begin();
								index != info.m_options.end(); ++index) {
			ARG->m_config.addOption(info.m_screen, index->first, index->second);
		}

		// update neighbors
		updateNeighbors(hwnd);
		enableScreensControls(hwnd);
		enableSaveControls(hwnd);
	}
}

static
void
editScreen(HWND hwnd)
{
	// get selected list item
	HWND child    = getItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST);
	LRESULT index = SendMessage(child, LB_GETCURSEL, 0, 0);
	if (index == LB_ERR) {
		// no selection
		return;
	}

	// fill in screen info
	CScreenInfo info;
	info.m_screen = ARG->m_screens[index];
	for (CConfig::all_const_iterator index = ARG->m_config.beginAll();
								index != ARG->m_config.endAll(); ++index) {
		if (CStringUtil::CaselessCmp::equal(index->second, info.m_screen) &&
			!CStringUtil::CaselessCmp::equal(index->second, index->first)) {
			info.m_aliases.push_back(index->first);
		}
	}
	const CConfig::CScreenOptions* options =
							ARG->m_config.getOptions(info.m_screen);
	if (options != NULL) {
		info.m_options = *options;
	}

	// save current info
	CScreenInfo oldInfo = info;

	// run dialog
	if (DialogBoxParam(s_instance, MAKEINTRESOURCE(IDD_ADD),
								hwnd, addDlgProc, (LPARAM)&info) != 0) {
		// replace screen
		ARG->m_screens[index] = info.m_screen;

		// remove old aliases
		for (CStringList::const_iterator index = oldInfo.m_aliases.begin();
								index != oldInfo.m_aliases.end(); ++index) {
			ARG->m_config.removeAlias(*index);
		}

		// replace name
		ARG->m_config.renameScreen(oldInfo.m_screen, info.m_screen);

		// add new aliases
		for (CStringList::const_iterator index = info.m_aliases.begin();
								index != info.m_aliases.end(); ++index) {
			ARG->m_config.addAlias(info.m_screen, *index);
		}

		// set options
		ARG->m_config.removeOptions(info.m_screen);
		for (CConfig::CScreenOptions::const_iterator
								index  = info.m_options.begin();
								index != info.m_options.end(); ++index) {
			ARG->m_config.addOption(info.m_screen, index->first, index->second);
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
		enableSaveControls(hwnd);
	}
}

static
void
removeScreen(HWND hwnd)
{
	// get selected list item
	HWND child    = getItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST);
	LRESULT index = SendMessage(child, LB_GETCURSEL, 0, 0);
	if (index == LB_ERR) {
		// no selection
		return;
	}

	// get screen name
	CString name = ARG->m_screens[index];

	// remove screen from list control
	SendMessage(child, LB_DELETESTRING, index, 0);

	// remove screen from screen list
	ARG->m_screens.erase(&ARG->m_screens[index]);

	// remove screen from config (this also removes aliases)
	ARG->m_config.removeScreen(name);

	// update neighbors
	updateNeighbors(hwnd);
	enableScreensControls(hwnd);
	enableSaveControls(hwnd);
}

static
void
changeNeighbor(HWND hwnd, HWND combo, EDirection direction)
{
	// get selected screen
	HWND child    = getItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST);
	LRESULT index = SendMessage(child, LB_GETCURSEL, 0, 0);
	if (index == LB_ERR) {
		// no selection
		return;
	}

	// get screen name
	CString screen = ARG->m_screens[index];

	// get selected neighbor
	index = SendMessage(combo, CB_GETCURSEL, 0, 0);

	// remove old connection
	ARG->m_config.disconnect(screen, direction);

	// add new connection
	if (index != LB_ERR && index != 0) {
		LRESULT size = SendMessage(combo, CB_GETLBTEXTLEN, index, 0);
		char* neighbor = new char[size + 1];
		SendMessage(combo, CB_GETLBTEXT, index, (LPARAM)neighbor);
		ARG->m_config.connect(screen, direction, CString(neighbor));
		delete[] neighbor;
	}

	enableSaveControls(hwnd);
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
	CString appPath = getAppPath(app);

	// put path to app in command line
	CString commandLine = "\"";
	commandLine += appPath;
	commandLine += "\" ";
	commandLine += cmdLine;

	// start child
	if (CreateProcess(NULL, (char*)commandLine.c_str(),
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
CString
getCommandLine(HWND hwnd, bool testing)
{
	CString cmdLine;

	// add constant testing args
	if (testing) {
		cmdLine += " -z --no-restart --no-daemon";
	}

	// can't start as service on NT
	else if (!CArchMiscWindows::isWindows95Family()) {
		cmdLine += " --no-daemon";
	}

	// get the server name
	CString server;
	bool isClient = isClientChecked(hwnd);
	if (isClient) {
		// check server name
		HWND child = getItem(hwnd, IDC_MAIN_CLIENT_SERVER_NAME_EDIT);
		server = getWindowText(child);
		if (!ARG->m_config.isValidScreenName(server)) {
			showError(hwnd, CStringUtil::format(
								getString(IDS_INVALID_SERVER_NAME).c_str(),
								server.c_str()));
			SetFocus(child);
			return CString();
		}

		// compare server name to local host.  a common error
		// is to provide the client's name for the server.  we
		// don't bother to check the addresses though that'd be
		// more accurate.
		if (CStringUtil::CaselessCmp::equal(ARCH->getHostName(), server)) {
			showError(hwnd, CStringUtil::format(
								getString(IDS_SERVER_IS_CLIENT).c_str(),
								server.c_str()));
			SetFocus(child);
			return CString();
		}
	}

	// debug level.  always include this.
	if (true) {
		HWND child = getItem(hwnd, IDC_MAIN_DEBUG);
		int debug  = (int)SendMessage(child, CB_GETCURSEL, 0, 0);

		// if testing then we force the debug level to be no less than
		// s_minTestDebug.   what's the point of testing if you can't
		// see the debugging info?
		if (testing && debug < s_minTestDebug) {
			debug = s_minTestDebug;
		}

		cmdLine    += " --debug ";
		cmdLine    += s_debugName[debug][1];
	}

	// add advanced options
	cmdLine += s_advancedOptions->getCommandLine(isClient, server);

	return cmdLine;
}

static
HANDLE
launchApp(HWND hwnd, bool testing, DWORD* threadID)
{
	// decide if client or server
	const bool isClient = isClientChecked(hwnd);
	const char* app = isClient ? CLIENT_APP : SERVER_APP;

	// prepare command line
	CString cmdLine = getCommandLine(hwnd, testing);
	if (cmdLine.empty()) {
		return NULL;
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
void
initMainWindow(HWND hwnd)
{
	// append version number to title
	CString titleFormat = getString(IDS_TITLE);
	setWindowText(hwnd, CStringUtil::format(titleFormat.c_str(), VERSION));

	// load configuration
	bool configLoaded = loadConfig(ARG->m_config);
	ARG->m_oldConfig = ARG->m_config;
	enableSaveControls(hwnd);

	// get settings from registry
	bool isServer = configLoaded;
	int debugLevel = s_defaultDebug;
	CString server;
	HKEY key = CArchMiscWindows::openKey(HKEY_CURRENT_USER, getSettingsPath());
	if (key != NULL) {
		if (isServer && CArchMiscWindows::hasValue(key, "isServer")) {
			isServer = (CArchMiscWindows::readValueInt(key, "isServer") != 0);
		}
		if (CArchMiscWindows::hasValue(key, "debug")) {
			debugLevel = static_cast<int>(
								CArchMiscWindows::readValueInt(key, "debug"));
			if (debugLevel < 0) {
				debugLevel = 0;
			}
			else if (debugLevel > CLog::kDEBUG2) {
				debugLevel = CLog::kDEBUG2;
			}
		}
		server = CArchMiscWindows::readValueString(key, "server");
		CArchMiscWindows::closeKey(key);
	}

	// choose client/server radio buttons
	HWND child;
	child = getItem(hwnd, IDC_MAIN_CLIENT_RADIO);
	setItemChecked(child, !isServer);
	child = getItem(hwnd, IDC_MAIN_SERVER_RADIO);
	setItemChecked(child, isServer);

	// set server name
	child = getItem(hwnd, IDC_MAIN_CLIENT_SERVER_NAME_EDIT);
	setWindowText(child, server);

	// if config is loaded then initialize server controls
	if (configLoaded) {
		int i = 1;
		child = getItem(hwnd, IDC_MAIN_SERVER_SCREENS_LIST);
		for (CConfig::const_iterator index = ARG->m_config.begin();
								index != ARG->m_config.end(); ++i, ++index) {
			ARG->m_screens.push_back(*index);
			CString item = CStringUtil::print("%d. %s", i, index->c_str());
			SendMessage(child, LB_ADDSTRING, 0, (LPARAM)item.c_str());
		}
	}

	// debug level
	child = getItem(hwnd, IDC_MAIN_DEBUG);
	for (unsigned int i = 0; i < sizeof(s_debugName) /
								sizeof(s_debugName[0]); ++i) {
		SendMessage(child, CB_ADDSTRING, 0, (LPARAM)s_debugName[i][0]);
	}
	SendMessage(child, CB_SETCURSEL, debugLevel, 0);

	// update neighbor combo boxes
	enableMainWindowControls(hwnd);
	updateNeighbors(hwnd);
}

static
void
saveMainWindow(HWND hwnd)
{
	HKEY key = CArchMiscWindows::openKey(HKEY_CURRENT_USER, getSettingsPath());
	if (key != NULL) {
		HWND child;
		child = getItem(hwnd, IDC_MAIN_CLIENT_SERVER_NAME_EDIT);
		CArchMiscWindows::setValue(key, "server", getWindowText(child));
		child = getItem(hwnd, IDC_MAIN_DEBUG);
		CArchMiscWindows::setValue(key, "debug",
								SendMessage(child, CB_GETCURSEL, 0, 0));
		CArchMiscWindows::setValue(key, "isServer",
								isClientChecked(hwnd) ? 0 : 1);
		CArchMiscWindows::closeKey(key);
	}
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

		// set title
		CString title;
		if (info->m_screen.empty()) {
			title = getString(IDS_ADD_SCREEN);
		}
		else {
			title = CStringUtil::format(
								getString(IDS_EDIT_SCREEN).c_str(),
								info->m_screen.c_str());
		}
		SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)title.c_str());

		// fill in screen name
		HWND child = getItem(hwnd, IDC_ADD_SCREEN_NAME_EDIT);
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
		child = getItem(hwnd, IDC_ADD_ALIASES_EDIT);
		SendMessage(child, WM_SETTEXT, 0, (LPARAM)aliases.c_str());

		// set options
		CConfig::CScreenOptions::const_iterator index;
		child = getItem(hwnd, IDC_ADD_HD_CAPS_CHECK);
		index = info->m_options.find(kOptionHalfDuplexCapsLock);
		setItemChecked(child, (index != info->m_options.end() &&
											index->second != 0));
		child = getItem(hwnd, IDC_ADD_HD_NUM_CHECK);
		index = info->m_options.find(kOptionHalfDuplexNumLock);
		setItemChecked(child, (index != info->m_options.end() &&
											index->second != 0));
		child = getItem(hwnd, IDC_ADD_HD_SCROLL_CHECK);
		index = info->m_options.find(kOptionHalfDuplexScrollLock);
		setItemChecked(child, (index != info->m_options.end() &&
											index->second != 0));

		// modifier options
		for (UInt32 i = 0; i < sizeof(s_modifiers) /
									sizeof(s_modifiers[0]); ++i) {
			child = getItem(hwnd, s_modifiers[i].m_ctrlID);

			// fill in options
			for (UInt32 j = 0; j < sizeof(s_modifiers) /
										sizeof(s_modifiers[0]); ++j) {
				SendMessage(child, CB_ADDSTRING, 0,
									(LPARAM)s_modifiers[j].m_name);
			}

			// choose current value
			index            = info->m_options.find(s_modifiers[i].m_optionID);
			KeyModifierID id = s_modifiers[i].m_modifierID;
			if (index != info->m_options.end()) {
				id = index->second;
			}
			SendMessage(child, CB_SETCURSEL, id - baseModifier, 0);
		}

		// dead corners
		UInt32 corners = 0;
		index = info->m_options.find(kOptionScreenSwitchCorners);
		if (index != info->m_options.end()) {
			corners = index->second;
		}
		child = getItem(hwnd, IDC_ADD_DC_TOP_LEFT);
		setItemChecked(child, (corners & kTopLeftMask) != 0);
		child = getItem(hwnd, IDC_ADD_DC_TOP_RIGHT);
		setItemChecked(child, (corners & kTopRightMask) != 0);
		child = getItem(hwnd, IDC_ADD_DC_BOTTOM_LEFT);
		setItemChecked(child, (corners & kBottomLeftMask) != 0);
		child = getItem(hwnd, IDC_ADD_DC_BOTTOM_RIGHT);
		setItemChecked(child, (corners & kBottomRightMask) != 0);
		index = info->m_options.find(kOptionScreenSwitchCornerSize);
		SInt32 size = 0;
		if (index != info->m_options.end()) {
			size = index->second;
		}
		char buffer[20];
		sprintf(buffer, "%d", size);
		child = getItem(hwnd, IDC_ADD_DC_SIZE);
		SendMessage(child, WM_SETTEXT, 0, (LPARAM)buffer);

		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: {
			CString newName;
			CStringList newAliases;

			// extract name and aliases
			HWND child = getItem(hwnd, IDC_ADD_SCREEN_NAME_EDIT);
			newName = getWindowText(child);
			child = getItem(hwnd, IDC_ADD_ALIASES_EDIT);
			tokenize(newAliases, getWindowText(child));

			// name must be valid
			if (!ARG->m_config.isValidScreenName(newName)) {
				showError(hwnd, CStringUtil::format(
								getString(IDS_INVALID_SCREEN_NAME).c_str(),
								newName.c_str()));
				return TRUE;
			}

			// aliases must be valid
			for (CStringList::const_iterator index = newAliases.begin();
								index != newAliases.end(); ++index) {
				if (!ARG->m_config.isValidScreenName(*index)) {
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
			if (ARG->m_config.isScreen(newName) &&
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
				if (ARG->m_config.isScreen(*index) &&
					!CStringUtil::CaselessCmp::equal(*index, info->m_screen) &&
					!isNameInList(info->m_aliases, *index)) {
					showError(hwnd, CStringUtil::format(
								getString(IDS_DUPLICATE_SCREEN_NAME).c_str(),
								index->c_str()));
					return TRUE;
				}
			}

			// dead corner size must be non-negative
			child = getItem(hwnd, IDC_ADD_DC_SIZE);
			CString valueString = getWindowText(child);
			int cornerSize = atoi(valueString.c_str());
			if (cornerSize < 0) {
				showError(hwnd, CStringUtil::format(
									getString(IDS_INVALID_CORNER_SIZE).c_str(),
									valueString.c_str()));
				SetFocus(child);
				return TRUE;
			}

			// save name data
			info->m_screen  = newName;
			info->m_aliases = newAliases;

			// save options
			child = getItem(hwnd, IDC_ADD_HD_CAPS_CHECK);
			if (isItemChecked(child)) {
				info->m_options[kOptionHalfDuplexCapsLock] = 1;
			}
			else {
				info->m_options.erase(kOptionHalfDuplexCapsLock);
			}
			child = getItem(hwnd, IDC_ADD_HD_NUM_CHECK);
			if (isItemChecked(child)) {
				info->m_options[kOptionHalfDuplexNumLock] = 1;
			}
			else {
				info->m_options.erase(kOptionHalfDuplexNumLock);
			}
			child = getItem(hwnd, IDC_ADD_HD_SCROLL_CHECK);
			if (isItemChecked(child)) {
				info->m_options[kOptionHalfDuplexScrollLock] = 1;
			}
			else {
				info->m_options.erase(kOptionHalfDuplexScrollLock);
			}

			// save modifier options
			for (UInt32 i = 0; i < sizeof(s_modifiers) /
										sizeof(s_modifiers[0]); ++i) {
				child            = getItem(hwnd, s_modifiers[i].m_ctrlID);
				KeyModifierID id = static_cast<KeyModifierID>(
									SendMessage(child, CB_GETCURSEL, 0, 0) +
										baseModifier);
				if (id != s_modifiers[i].m_modifierID) {
					info->m_options[s_modifiers[i].m_optionID] = id;
				}
				else {
					info->m_options.erase(s_modifiers[i].m_optionID);
				}
			}

			// save dead corner options
			UInt32 corners = 0;
			if (isItemChecked(getItem(hwnd, IDC_ADD_DC_TOP_LEFT))) {
				corners |= kTopLeftMask;
			}
			if (isItemChecked(getItem(hwnd, IDC_ADD_DC_TOP_RIGHT))) {
				corners |= kTopRightMask;
			}
			if (isItemChecked(getItem(hwnd, IDC_ADD_DC_BOTTOM_LEFT))) {
				corners |= kBottomLeftMask;
			}
			if (isItemChecked(getItem(hwnd, IDC_ADD_DC_BOTTOM_RIGHT))) {
				corners |= kBottomRightMask;
			}
			info->m_options[kOptionScreenSwitchCorners]    = corners;
			info->m_options[kOptionScreenSwitchCornerSize] = cornerSize;

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
			if (ARG->m_config != ARG->m_oldConfig) {
				if (!askVerify(hwnd, getString(IDS_UNSAVED_DATA_REALLY_QUIT))) {
					return 0;
				}
			}

			// quit
			PostQuitMessage(0);
			return 0;

		case IDOK:
		case IDC_MAIN_TEST: {
			// note if testing
			const bool testing = (LOWORD(wParam) == IDC_MAIN_TEST);

			// save data
			if (ARG->m_config != ARG->m_oldConfig) {
				if (!saveConfig(ARG->m_config, false)) {
					showError(hwnd, CStringUtil::format(
								getString(IDS_SAVE_FAILED).c_str(),
								getErrorString(GetLastError()).c_str()));
					return 0;
				}
				ARG->m_oldConfig = ARG->m_config;
				enableSaveControls(hwnd);
			}

			// launch child app
			DWORD threadID;
			HANDLE thread = launchApp(hwnd, testing, &threadID);
			if (thread == NULL) {
				return 0;
			}

			// handle child program
			if (testing) {
				// wait for process to stop, allowing the user to kill it
				waitForChild(hwnd, thread, threadID);

				// clean up
				CloseHandle(thread);
			}
			else {
				// don't need thread handle
				CloseHandle(thread);

				// notify of success
				askOkay(hwnd, getString(IDS_STARTED_TITLE),
								getString(IDS_STARTED));

				// quit
				PostQuitMessage(0);
			}
			return 0;
		}

		case IDC_MAIN_AUTOSTART: {
			// construct command line
			CString cmdLine = getCommandLine(hwnd, false);
			if (!cmdLine.empty()) {
				// run dialog
				CAutoStart autoStart(hwnd,
							isClientChecked(hwnd) ? NULL : &ARG->m_config,
							cmdLine);
				autoStart.doModal();
				if (autoStart.wasUserConfigSaved()) {
					ARG->m_oldConfig = ARG->m_config;
					enableSaveControls(hwnd);
				}
			}
			return 0;
		}

		case IDC_MAIN_SAVE:
			if (!saveConfig(ARG->m_config, false)) {
				showError(hwnd, CStringUtil::format(
								getString(IDS_SAVE_FAILED).c_str(),
								getErrorString(GetLastError()).c_str()));
			}
			else {
				ARG->m_oldConfig = ARG->m_config;
				enableSaveControls(hwnd);
			}
			return 0;

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

		case IDC_MAIN_OPTIONS:
			s_globalOptions->doModal();
			enableSaveControls(hwnd);
			break;

		case IDC_MAIN_ADVANCED:
			s_advancedOptions->doModal(isClientChecked(hwnd));
			enableSaveControls(hwnd);
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
	CArch arch(instance);
	CLOG;
	CArgs args;

	s_instance = instance;

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
	HWND mainWindow = CreateDialog(s_instance,
							MAKEINTRESOURCE(IDD_MAIN), 0, NULL);

	// prep windows
	initMainWindow(mainWindow);
	s_globalOptions = new CGlobalOptions(mainWindow, &ARG->m_config);
	s_advancedOptions = new CAdvancedOptions(mainWindow, &ARG->m_config);

	// show window
	ShowWindow(mainWindow, nCmdShow);

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
			if (!IsDialogMessage(mainWindow, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			break;
		}
	} while (!done);

	// save values to registry
	saveMainWindow(mainWindow);

	return msg.wParam;
}
