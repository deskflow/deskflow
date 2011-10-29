/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CConfig.h"
#include "KeyTypes.h"
#include "OptionTypes.h"
#include "ProtocolTypes.h"
#include "CLog.h"
#include "CStringUtil.h"
#include "CArch.h"
#include "CArchMiscWindows.h"
#include "XArch.h"
#include "Version.h"
#include "stdvector.h"
#include "resource.h"

// these must come after the above because it includes windows.h
#include "LaunchUtil.h"
#include "CAddScreen.h"
#include "CAdvancedOptions.h"
#include "CAutoStart.h"
#include "CGlobalOptions.h"
#include "CHotkeyOptions.h"
#include "CInfo.h"
#include "CScreensLinks.h"

typedef std::vector<CString> CStringList;

class CChildWaitInfo {
public:
	HWND				m_dialog;
	HANDLE				m_child;
	DWORD				m_childID;
	HANDLE				m_ready;
	HANDLE				m_stop;
};

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
static CHotkeyOptions*		s_hotkeyOptions   = NULL;
static CScreensLinks*		s_screensLinks    = NULL;
static CInfo*				s_info            = NULL;

static bool		s_userConfig = true;
static time_t	s_configTime = 0;
static CConfig	s_lastConfig;

static const TCHAR* s_mainClass   = TEXT("GoSynergy");
static const TCHAR* s_layoutClass = TEXT("SynergyLayout");

enum SaveMode {
	SAVE_QUITING,
	SAVE_NORMAL,
	SAVE_QUIET
};

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
	CStringList			m_screens;
};

CArgs*					CArgs::s_instance = NULL;


static
BOOL CALLBACK
addDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static
bool
isClientChecked(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_MAIN_CLIENT_RADIO);
	return isItemChecked(child);
}

static
void
enableMainWindowControls(HWND hwnd)
{
	bool client = isClientChecked(hwnd);
	enableItem(hwnd, IDC_MAIN_CLIENT_SERVER_NAME_LABEL, client);
	enableItem(hwnd, IDC_MAIN_CLIENT_SERVER_NAME_EDIT, client);
	enableItem(hwnd, IDC_MAIN_SERVER_SCREENS_LABEL, !client);
	enableItem(hwnd, IDC_MAIN_SCREENS, !client);
	enableItem(hwnd, IDC_MAIN_OPTIONS, !client);
	enableItem(hwnd, IDC_MAIN_HOTKEYS, !client);
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
getCommandLine(HWND hwnd, bool testing, bool silent)
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
			if (!silent) {
				showError(hwnd, CStringUtil::format(
								getString(IDS_INVALID_SERVER_NAME).c_str(),
								server.c_str()));
			}
			SetFocus(child);
			return CString();
		}

		// compare server name to local host.  a common error
		// is to provide the client's name for the server.  we
		// don't bother to check the addresses though that'd be
		// more accurate.
		if (CStringUtil::CaselessCmp::equal(ARCH->getHostName(), server)) {
			if (!silent) {
				showError(hwnd, CStringUtil::format(
								getString(IDS_SERVER_IS_CLIENT).c_str(),
								server.c_str()));
			}
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
bool
launchApp(HWND hwnd, bool testing, HANDLE* thread, DWORD* threadID)
{
	if (thread != NULL) {
		*thread = NULL;
	}
	if (threadID != NULL) {
		*threadID = 0;
	}

	// start daemon if it's installed and we're not testing
	if (!testing && CAutoStart::startDaemon()) {
		return true;
	}

	// decide if client or server
	const bool isClient = isClientChecked(hwnd);
	const char* app = isClient ? CLIENT_APP : SERVER_APP;

	// prepare command line
	CString cmdLine = getCommandLine(hwnd, testing, false);
	if (cmdLine.empty()) {
		return false;
	}

	// start child
	PROCESS_INFORMATION procInfo;
	if (!execApp(app, cmdLine, &procInfo)) {
		showError(hwnd, CStringUtil::format(
								getString(IDS_STARTUP_FAILED).c_str(),
								getErrorString(GetLastError()).c_str()));
		return false;
	}

	// don't need process handle
	CloseHandle(procInfo.hProcess);

	// save thread handle and thread ID if desired
	if (thread != NULL) {
		*thread = procInfo.hThread;
	}
	if (threadID != NULL) {
		*threadID = procInfo.dwThreadId;
	}

	return true;
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
								(DLGPROC)waitDlgProc, (LPARAM)&info);

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
	setWindowText(hwnd, CStringUtil::format(titleFormat.c_str(), kApplication, kVersion));

	// load configuration
	bool configLoaded =
		loadConfig(ARG->m_config, s_configTime, s_userConfig);
	if (configLoaded) {
		s_lastConfig = ARG->m_config;
	}

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
			else if (debugLevel > kDEBUG2) {
				debugLevel = kDEBUG2;
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

	// debug level
	child = getItem(hwnd, IDC_MAIN_DEBUG);
	for (unsigned int i = 0; i < sizeof(s_debugName) /
								sizeof(s_debugName[0]); ++i) {
		SendMessage(child, CB_ADDSTRING, 0, (LPARAM)s_debugName[i][0]);
	}
	SendMessage(child, CB_SETCURSEL, debugLevel, 0);

	// update controls
	enableMainWindowControls(hwnd);
}

static
bool
saveMainWindow(HWND hwnd, SaveMode mode, CString* cmdLineOut = NULL)
{
	DWORD errorID = 0;
	CString arg;
	CString cmdLine;

	// save dialog state
	bool isClient = isClientChecked(hwnd);
	HKEY key = CArchMiscWindows::addKey(HKEY_CURRENT_USER, getSettingsPath());
	if (key != NULL) {
		HWND child;
		child = getItem(hwnd, IDC_MAIN_CLIENT_SERVER_NAME_EDIT);
		CArchMiscWindows::setValue(key, "server", getWindowText(child));
		child = getItem(hwnd, IDC_MAIN_DEBUG);
		CArchMiscWindows::setValue(key, "debug",
								(DWORD)SendMessage(child, CB_GETCURSEL, 0, 0));
		CArchMiscWindows::setValue(key, "isServer", isClient ? 0 : 1);
		CArchMiscWindows::closeKey(key);
	}

	// save user's configuration
	if (!s_userConfig || ARG->m_config != s_lastConfig) {
		time_t t;
		if (!saveConfig(ARG->m_config, false, t)) {
			errorID = IDS_SAVE_FAILED;
			arg     = getErrorString(GetLastError());
			goto failed;
		}
		if (s_userConfig) {
			s_configTime = t;
			s_lastConfig = ARG->m_config;
		}
	}

	// save autostart configuration
	if (CAutoStart::isDaemonInstalled()) {
		if (s_userConfig || ARG->m_config != s_lastConfig) {
			time_t t;
			if (!saveConfig(ARG->m_config, true, t)) {
				errorID = IDS_AUTOSTART_SAVE_FAILED;
				arg     = getErrorString(GetLastError());
				goto failed;
			}
			if (!s_userConfig) {
				s_configTime = t;
				s_lastConfig = ARG->m_config;
			}
		}
	}

	// get autostart command
	cmdLine = getCommandLine(hwnd, false, mode == SAVE_QUITING);
	if (cmdLineOut != NULL) {
		*cmdLineOut = cmdLine;
	}
	if (cmdLine.empty()) {
		return (mode == SAVE_QUITING);
	}

	// save autostart command
	if (CAutoStart::isDaemonInstalled()) {
		try {
			CAutoStart::reinstallDaemon(isClient, cmdLine);
			CAutoStart::uninstallDaemons(!isClient);
		}
		catch (XArchDaemon& e) {
			errorID = IDS_INSTALL_GENERIC_ERROR;
			arg     = e.what();
			goto failed;
		}
	}

	return true;

failed:
	CString errorMessage =
		CStringUtil::format(getString(errorID).c_str(), arg.c_str());
	if (mode == SAVE_QUITING) {
		errorMessage += "\n";
		errorMessage += getString(IDS_UNSAVED_DATA_REALLY_QUIT);
		if (askVerify(hwnd, errorMessage)) {
			return true;
		}
	}
	else if (mode == SAVE_NORMAL) {
		showError(hwnd, errorMessage);
	}
	return false;
}

static
LRESULT CALLBACK
mainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_INACTIVE) {
			// activated

			// see if the configuration changed
			if (isConfigNewer(s_configTime, s_userConfig)) {
				CString message2 = getString(IDS_CONFIG_CHANGED);
				if (askVerify(hwnd, message2)) {
					time_t configTime;
					bool userConfig;
					CConfig newConfig;
					if (loadConfig(newConfig, configTime, userConfig) &&
						userConfig == s_userConfig) {
						ARG->m_config = newConfig;
						s_lastConfig  = ARG->m_config;
					}
					else {
						message2 = getString(IDS_LOAD_FAILED);
						showError(hwnd, message2);
						s_lastConfig = CConfig();
					}
				}
			}
		}
		else {
			// deactivated;  write configuration
			if (!isShowingDialog()) {
				saveMainWindow(hwnd, SAVE_QUIET);
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			// save data
			if (saveMainWindow(hwnd, SAVE_QUITING)) {
				// quit
				PostQuitMessage(0);
			}
			return 0;

		case IDOK:
		case IDC_MAIN_TEST: {
			// note if testing
			const bool testing = (LOWORD(wParam) == IDC_MAIN_TEST);

			// save data
			if (saveMainWindow(hwnd, SAVE_NORMAL)) {
				// launch child app
				DWORD threadID;
				HANDLE thread;
				if (!launchApp(hwnd, testing, &thread, &threadID)) {
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
					if (thread != NULL) {
						CloseHandle(thread);
					}

					// notify of success: now disabled - it's silly to notify a success
					//askOkay(hwnd, getString(IDS_STARTED_TITLE), getString(IDS_STARTED));

					// quit
					PostQuitMessage(0);
				}
			}
			return 0;
		}

		case IDC_MAIN_AUTOSTART: {
			CString cmdLine;
			if (saveMainWindow(hwnd, SAVE_NORMAL, &cmdLine)) {
				// run dialog
				CAutoStart autoStart(hwnd, !isClientChecked(hwnd), cmdLine);
				autoStart.doModal();
			}
			return 0;
		}

		case IDC_MAIN_CLIENT_RADIO:
		case IDC_MAIN_SERVER_RADIO:
			enableMainWindowControls(hwnd);
			return 0;

		case IDC_MAIN_SCREENS:
			s_screensLinks->doModal();
			break;

		case IDC_MAIN_OPTIONS:
			s_globalOptions->doModal();
			break;

		case IDC_MAIN_ADVANCED:
			s_advancedOptions->doModal(isClientChecked(hwnd));
			break;

		case IDC_MAIN_HOTKEYS:
			s_hotkeyOptions->doModal();
			break;

		case IDC_MAIN_INFO:
			s_info->doModal();
			break;
		}

	default:
		break;
	}
	return DefDlgProc(hwnd, message, wParam, lParam);
}

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE, LPSTR cmdLine, int nCmdShow)
{
	CArch arch(instance);
	CLOG;
	CArgs args;

	s_instance = instance;

	// if "/uninstall" is on the command line then just stop and
	// uninstall the service and quit.  this is the only option
	// but we ignore any others.
	if (CString(cmdLine).find("/uninstall") != CString::npos) {
		CAutoStart::uninstallDaemons(false);
		CAutoStart::uninstallDaemons(true);
		return 0;
	}

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
	s_globalOptions   = new CGlobalOptions(mainWindow, &ARG->m_config);
	s_advancedOptions = new CAdvancedOptions(mainWindow, &ARG->m_config);
	s_hotkeyOptions   = new CHotkeyOptions(mainWindow, &ARG->m_config); 
	s_screensLinks    = new CScreensLinks(mainWindow, &ARG->m_config);
	s_info            = new CInfo(mainWindow);

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

	return (int)msg.wParam;
}
