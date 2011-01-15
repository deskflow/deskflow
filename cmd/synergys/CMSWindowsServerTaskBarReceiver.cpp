/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include "CMSWindowsServerTaskBarReceiver.h"
#include "CServer.h"
#include "CMSWindowsClipboard.h"
#include "IEventQueue.h"
#include "LogOutputters.h"
#include "BasicTypes.h"
#include "CArch.h"
#include "CArchTaskBarWindows.h"
#include "resource.h"
#include "CArchMiscWindows.h"
#include "CMSWindowsScreen.h"

//
// CMSWindowsServerTaskBarReceiver
//

const UINT CMSWindowsServerTaskBarReceiver::s_stateToIconID[kMaxState] =
{
	IDI_TASKBAR_NOT_RUNNING,
	IDI_TASKBAR_NOT_WORKING,
	IDI_TASKBAR_NOT_CONNECTED,
	IDI_TASKBAR_CONNECTED
};

CMSWindowsServerTaskBarReceiver::CMSWindowsServerTaskBarReceiver(
				HINSTANCE appInstance, const CBufferedLogOutputter* logBuffer) :
	CServerTaskBarReceiver(),
	m_appInstance(appInstance),
	m_window(NULL),
	m_logBuffer(logBuffer)
{
	for (UInt32 i = 0; i < kMaxState; ++i) {
		m_icon[i] = loadIcon(s_stateToIconID[i]);
	}
	m_menu = LoadMenu(m_appInstance, MAKEINTRESOURCE(IDR_TASKBAR));

	// don't create the window yet.  we'll create it on demand.  this
	// has the side benefit of being created in the thread used for
	// the task bar.  that's good because it means the existence of
	// the window won't prevent changing the main thread's desktop.

	// add ourself to the task bar
	ARCH->addReceiver(this);
}

void
CMSWindowsServerTaskBarReceiver::cleanup()
{
	ARCH->removeReceiver(this);
	for (UInt32 i = 0; i < kMaxState; ++i) {
		deleteIcon(m_icon[i]);
	}
	DestroyMenu(m_menu);
	destroyWindow();
}

CMSWindowsServerTaskBarReceiver::~CMSWindowsServerTaskBarReceiver()
{
	cleanup();
}

void
CMSWindowsServerTaskBarReceiver::showStatus()
{
	// create the window
	createWindow();

	// lock self while getting status
	lock();

	// get the current status
	std::string status = getToolTip();

	// get the connect clients, if any
	const CClients& clients = getClients();

	// done getting status
	unlock();

	// update dialog
	HWND child = GetDlgItem(m_window, IDC_TASKBAR_STATUS_STATUS);
	SendMessage(child, WM_SETTEXT, 0, (LPARAM)status.c_str());
	child = GetDlgItem(m_window, IDC_TASKBAR_STATUS_CLIENTS);
	SendMessage(child, LB_RESETCONTENT, 0, 0);
	for (CClients::const_iterator index = clients.begin();
							index != clients.end(); ) {
		const char* client = index->c_str();
		if (++index == clients.end()) {
			SendMessage(child, LB_ADDSTRING, 0, (LPARAM)client);
		}
		else {
			SendMessage(child, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)client);
		}
	}

	if (!IsWindowVisible(m_window)) {
		// position it by the mouse
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		RECT windowRect;
		GetWindowRect(m_window, &windowRect);
		int  x = cursorPos.x;
		int  y = cursorPos.y;
		int fw = GetSystemMetrics(SM_CXDLGFRAME);
		int fh = GetSystemMetrics(SM_CYDLGFRAME);
		int ww = windowRect.right  - windowRect.left;
		int wh = windowRect.bottom - windowRect.top;
		int sw = GetSystemMetrics(SM_CXFULLSCREEN);
		int sh = GetSystemMetrics(SM_CYFULLSCREEN);
		if (fw < 1) {
			fw = 1;
		}
		if (fh < 1) {
			fh = 1;
		}
		if (x + ww - fw > sw) {
			x -= ww - fw;
		}
		else {
			x -= fw;
		}
		if (x < 0) {
			x = 0;
		}
		if (y + wh - fh > sh) {
			y -= wh - fh;
		}
		else {
			y -= fh;
		}
		if (y < 0) {
			y = 0;
		}
		SetWindowPos(m_window, HWND_TOPMOST, x, y, ww, wh,
							SWP_SHOWWINDOW);
	}
}

void
CMSWindowsServerTaskBarReceiver::runMenu(int x, int y)
{
	// do popup menu.  we need a window to pass to TrackPopupMenu().
	// the SetForegroundWindow() and SendMessage() calls around
	// TrackPopupMenu() are to get the menu to be dismissed when
	// another window gets activated and are just one of those
	// win32 weirdnesses.
	createWindow();
	SetForegroundWindow(m_window);
	HMENU menu = GetSubMenu(m_menu, 0);
	SetMenuDefaultItem(menu, IDC_TASKBAR_STATUS, FALSE);
	HMENU logLevelMenu = GetSubMenu(menu, 3);
	CheckMenuRadioItem(logLevelMenu, 0, 6,
							CLOG->getFilter() - CLog::kERROR, MF_BYPOSITION);
	int n = TrackPopupMenu(menu,
							TPM_NONOTIFY |
							TPM_RETURNCMD |
							TPM_LEFTBUTTON |
							TPM_RIGHTBUTTON,
							x, y, 0, m_window, NULL);
	SendMessage(m_window, WM_NULL, 0, 0);

	// perform the requested operation
	switch (n) {
	case IDC_TASKBAR_STATUS:
		showStatus();
		break;

	case IDC_TASKBAR_LOG:
		copyLog();
		break;

	case IDC_TASKBAR_SHOW_LOG:
		ARCH->showConsole(true);
		break;

	case IDC_RELOAD_CONFIG:
		EVENTQUEUE->addEvent(CEvent(getReloadConfigEvent(),
							IEventQueue::getSystemTarget()));
		break;

	case IDC_FORCE_RECONNECT:
		EVENTQUEUE->addEvent(CEvent(getForceReconnectEvent(),
							IEventQueue::getSystemTarget()));
		break;

	case ID_SYNERGY_RESETSERVER:
		EVENTQUEUE->addEvent(CEvent(getResetServerEvent(),
							IEventQueue::getSystemTarget()));
		break;

	case IDC_TASKBAR_LOG_LEVEL_ERROR:
		CLOG->setFilter(CLog::kERROR);
		break;

	case IDC_TASKBAR_LOG_LEVEL_WARNING:
		CLOG->setFilter(CLog::kWARNING);
		break;

	case IDC_TASKBAR_LOG_LEVEL_NOTE:
		CLOG->setFilter(CLog::kNOTE);
		break;

	case IDC_TASKBAR_LOG_LEVEL_INFO:
		CLOG->setFilter(CLog::kINFO);
		break;

	case IDC_TASKBAR_LOG_LEVEL_DEBUG:
		CLOG->setFilter(CLog::kDEBUG);
		break;

	case IDC_TASKBAR_LOG_LEVEL_DEBUG1:
		CLOG->setFilter(CLog::kDEBUG1);
		break;

	case IDC_TASKBAR_LOG_LEVEL_DEBUG2:
		CLOG->setFilter(CLog::kDEBUG2);
		break;

	case IDC_TASKBAR_QUIT:
		quit();
		break;
	}
}

void
CMSWindowsServerTaskBarReceiver::primaryAction()
{
	showStatus();
}

const IArchTaskBarReceiver::Icon
CMSWindowsServerTaskBarReceiver::getIcon() const
{
	return reinterpret_cast<Icon>(m_icon[getStatus()]);
}

void
CMSWindowsServerTaskBarReceiver::copyLog() const
{
	if (m_logBuffer != NULL) {
		// collect log buffer
		CString data;
		for (CBufferedLogOutputter::const_iterator index = m_logBuffer->begin();
								index != m_logBuffer->end(); ++index) {
			data += *index;
			data += "\n";
		}

		// copy log to clipboard
		if (!data.empty()) {
			CMSWindowsClipboard clipboard(m_window);
			clipboard.open(0);
			clipboard.emptyUnowned();
			clipboard.add(IClipboard::kText, data);
			clipboard.close();
		}
	}
}

void
CMSWindowsServerTaskBarReceiver::onStatusChanged()
{
	if (IsWindowVisible(m_window)) {
		showStatus();
	}
}

HICON
CMSWindowsServerTaskBarReceiver::loadIcon(UINT id)
{
	HANDLE icon = LoadImage(m_appInstance,
							MAKEINTRESOURCE(id),
							IMAGE_ICON,
							0, 0,
							LR_DEFAULTCOLOR);
	return reinterpret_cast<HICON>(icon);
}

void
CMSWindowsServerTaskBarReceiver::deleteIcon(HICON icon)
{
	if (icon != NULL) {
		DestroyIcon(icon);
	}
}

void
CMSWindowsServerTaskBarReceiver::createWindow()
{
	// ignore if already created
	if (m_window != NULL) {
		return;
	}

	// get the status dialog
	m_window = CreateDialogParam(m_appInstance,
							MAKEINTRESOURCE(IDD_TASKBAR_STATUS),
							NULL,
							(DLGPROC)&CMSWindowsServerTaskBarReceiver::staticDlgProc,
							reinterpret_cast<LPARAM>(
								reinterpret_cast<void*>(this)));

	// window should appear on top of everything, including (especially)
	// the task bar.
	LONG_PTR style = GetWindowLongPtr(m_window, GWL_EXSTYLE);
	style |= WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
	SetWindowLongPtr(m_window, GWL_EXSTYLE, style);

	// tell the task bar about this dialog
	CArchTaskBarWindows::addDialog(m_window);
}

void
CMSWindowsServerTaskBarReceiver::destroyWindow()
{
	if (m_window != NULL) {
		CArchTaskBarWindows::removeDialog(m_window);
		DestroyWindow(m_window);
		m_window = NULL;
	}
}

BOOL
CMSWindowsServerTaskBarReceiver::dlgProc(HWND hwnd,
							UINT msg, WPARAM wParam, LPARAM)
{
	switch (msg) {
	case WM_INITDIALOG:
		// use default focus
		return TRUE;

	case WM_ACTIVATE:
		// hide when another window is activated
		if (LOWORD(wParam) == WA_INACTIVE) {
			ShowWindow(hwnd, SW_HIDE);
		}
		break;
	}
	return FALSE;
}

BOOL CALLBACK
CMSWindowsServerTaskBarReceiver::staticDlgProc(HWND hwnd,
							UINT msg, WPARAM wParam, LPARAM lParam)
{
	// if msg is WM_INITDIALOG, extract the CMSWindowsServerTaskBarReceiver*
	// and put it in the extra window data then forward the call.
	CMSWindowsServerTaskBarReceiver* self = NULL;
	if (msg == WM_INITDIALOG) {
		self = reinterpret_cast<CMSWindowsServerTaskBarReceiver*>(
							reinterpret_cast<void*>(lParam));
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
	}
	else {
		// get the extra window data and forward the call
		LONG data = (LONG)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (data != 0) {
			self = reinterpret_cast<CMSWindowsServerTaskBarReceiver*>(
							reinterpret_cast<void*>(data));
		}
	}

	// forward the message
	if (self != NULL) {
		return self->dlgProc(hwnd, msg, wParam, lParam);
	}
	else {
		return (msg == WM_INITDIALOG) ? TRUE : FALSE;
	}
}

IArchTaskBarReceiver*
createTaskBarReceiver(const CBufferedLogOutputter* logBuffer)
{
	CArchMiscWindows::setIcons(
		(HICON)LoadImage(CArchMiscWindows::instanceWin32(),
		MAKEINTRESOURCE(IDI_SYNERGY),
		IMAGE_ICON,
		32, 32, LR_SHARED),
		(HICON)LoadImage(CArchMiscWindows::instanceWin32(),
		MAKEINTRESOURCE(IDI_SYNERGY),
		IMAGE_ICON,
		16, 16, LR_SHARED));

	return new CMSWindowsServerTaskBarReceiver(
		CMSWindowsScreen::getInstance(), logBuffer);
}
