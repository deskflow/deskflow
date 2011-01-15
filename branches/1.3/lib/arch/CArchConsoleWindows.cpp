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

#include "CArchConsoleWindows.h"
#include "IArchMultithread.h"
#include "CArch.h"
#include "CArchMiscWindows.h"
#include <richedit.h>

#define SYNERGY_MSG_CONSOLE_OPEN	WM_APP + 0x0021
#define SYNERGY_MSG_CONSOLE_CLOSE	WM_APP + 0x0022
#define SYNERGY_MSG_CONSOLE_SHOW	WM_APP + 0x0023
#define SYNERGY_MSG_CONSOLE_WRITE	WM_APP + 0x0024
#define SYNERGY_MSG_CONSOLE_CLEAR	WM_APP + 0x0025
#define TWIPS_PER_POINT		20

//
// CArchConsoleWindows
//

CArchConsoleWindows*	CArchConsoleWindows::s_instance    = NULL;
HINSTANCE				CArchConsoleWindows::s_appInstance = NULL;

CArchConsoleWindows::CArchConsoleWindows(void* appInstance) :
	m_show(false),
	m_maxLines(1000),
	m_numCharacters(0),
	m_maxCharacters(65536)
{
	// save the singleton instance
	s_instance    = this;

	// save app instance
	s_appInstance = reinterpret_cast<HINSTANCE>(appInstance);

	// we need a mutex
	m_mutex       = ARCH->newMutex();

	// and a condition variable which uses the above mutex
	m_ready       = false;
	m_condVar     = ARCH->newCondVar();

	// we're going to want to get a result from the thread we're
	// about to create to know if it initialized successfully.
	// so we lock the condition variable.
	ARCH->lockMutex(m_mutex);

	// open a window and run an event loop in a separate thread.
	// this has to happen in a separate thread because if we
	// create a window on the current desktop with the current
	// thread then the current thread won't be able to switch
	// desktops if it needs to.
	m_thread      = ARCH->newThread(&CArchConsoleWindows::threadEntry, this);

	// wait for child thread
	while (!m_ready) {
		ARCH->waitCondVar(m_condVar, m_mutex, -1.0);
	}

	// ready
	ARCH->unlockMutex(m_mutex);

}

CArchConsoleWindows::~CArchConsoleWindows()
{
	if (m_thread != NULL) {
		PostMessage(m_hwnd, WM_QUIT, 0, 0);
		ARCH->wait(m_thread, -1.0);
		ARCH->closeThread(m_thread);
	}
	ARCH->closeCondVar(m_condVar);
	ARCH->closeMutex(m_mutex);
	s_instance = NULL;
}

void
CArchConsoleWindows::openConsole(const char* title)
{
	SetWindowText(m_frame, title);
	SendMessage(m_frame, SYNERGY_MSG_CONSOLE_OPEN, 0, 0);
}

void
CArchConsoleWindows::closeConsole()
{
	SendMessage(m_frame, SYNERGY_MSG_CONSOLE_CLOSE, 0, 0);
	SendMessage(m_frame, SYNERGY_MSG_CONSOLE_CLEAR, 0, 0);
}

void
CArchConsoleWindows::showConsole(bool showIfEmpty)
{
	SendMessage(m_frame, SYNERGY_MSG_CONSOLE_SHOW, showIfEmpty ? 1 : 0, 0);
}

void
CArchConsoleWindows::writeConsole(const char* str)
{
	SendMessage(m_frame, SYNERGY_MSG_CONSOLE_WRITE,
								reinterpret_cast<WPARAM>(str), 0);
}

const char*
CArchConsoleWindows::getNewlineForConsole()
{
	return "\r\n";
}

void
CArchConsoleWindows::clearBuffer()
{
	m_buffer.clear();
	m_numCharacters = 0;
	SetWindowText(m_hwnd, "");
}

void
CArchConsoleWindows::appendBuffer(const char* msg)
{
	bool wasEmpty = m_buffer.empty();

	// get current selection
	CHARRANGE selection;
	SendMessage(m_hwnd, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&selection));

	// remove tail of buffer
	size_t removedCharacters = 0;
	while (m_buffer.size() >= m_maxLines) {
		removedCharacters += m_buffer.front().size();
		m_buffer.pop_front();
	}

	// remove lines from top of control
	if (removedCharacters > 0) {
		CHARRANGE range;
		range.cpMin = 0;
		range.cpMax = static_cast<LONG>(removedCharacters);
		SendMessage(m_hwnd, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
		SendMessage(m_hwnd, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(""));

		// adjust selection
		if (selection.cpMin < static_cast<LONG>(removedCharacters) ||
			selection.cpMax < static_cast<LONG>(removedCharacters)) {
			selection.cpMin = 0;
			selection.cpMax = 0;
		}
		else {
			selection.cpMin -= static_cast<LONG>(removedCharacters);
			selection.cpMax -= static_cast<LONG>(removedCharacters);
		}

		m_numCharacters -= removedCharacters;
	}

	// append message
	m_buffer.push_back(msg);
	size_t newNumCharacters = m_numCharacters + m_buffer.back().size();

	// add line to bottom of control
	if (newNumCharacters > m_maxCharacters) {
		m_maxCharacters = newNumCharacters;
		SendMessage(m_hwnd, EM_EXLIMITTEXT, 0, m_maxCharacters);
	}
	CHARRANGE range;
	range.cpMin = (LONG)m_numCharacters;
	range.cpMax = (LONG)m_numCharacters;
	SendMessage(m_hwnd, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
	SendMessage(m_hwnd, EM_REPLACESEL, FALSE,
							reinterpret_cast<LPARAM>(m_buffer.back().c_str()));

	// adjust selection
	bool atEnd = false;
	if (selection.cpMax == static_cast<LONG>(m_numCharacters)) {
		selection.cpMin = static_cast<LONG>(newNumCharacters);
		selection.cpMax = static_cast<LONG>(newNumCharacters);
		atEnd = true;
	}

	// restore the selection
	SendMessage(m_hwnd, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&selection));
	if (atEnd) {
		SendMessage(m_hwnd, EM_SCROLLCARET, 0, 0);
	}

	if (wasEmpty && m_show) {
		ShowWindow(m_frame, TRUE);
	}

	m_numCharacters = newNumCharacters;
}

void
CArchConsoleWindows::setSize(int width, int height)
{
	DWORD style   = GetWindowLong(m_frame, GWL_STYLE);
	DWORD exStyle = GetWindowLong(m_frame, GWL_EXSTYLE);
	RECT rect;
	rect.left   = 100;
	rect.top    = 100;
	rect.right  = rect.left + width * m_wChar;
	rect.bottom = rect.top + height * m_hChar;
	AdjustWindowRectEx(&rect, style, FALSE, exStyle);
	SetWindowPos(m_frame, NULL, 0, 0, rect.right - rect.left,
								rect.bottom - rect.top,
								SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
}

LRESULT
CArchConsoleWindows::wndProc(HWND hwnd,
				UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_CLOSE:
		ShowWindow(m_frame, FALSE);
		m_show = false;
		return 0;

	case SYNERGY_MSG_CONSOLE_OPEN:
		return 0;

	case SYNERGY_MSG_CONSOLE_CLOSE:
		SendMessage(m_frame, WM_CLOSE, 0, 0);
		m_show = false;
		return 0;

	case SYNERGY_MSG_CONSOLE_SHOW:
		m_show = true;
		if (wParam != 0 || !m_buffer.empty()) {
			ShowWindow(m_frame, TRUE);
		}
		return 0;

	case SYNERGY_MSG_CONSOLE_WRITE:
		appendBuffer(reinterpret_cast<const char*>(wParam));
		return 0;

	case SYNERGY_MSG_CONSOLE_CLEAR:
		clearBuffer();
		return 0;

	case WM_SIZE:
		if (hwnd == m_frame) {
			MoveWindow(m_hwnd, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		}
		break;

	case WM_SIZING:
		if (hwnd == m_frame) {
			// get window vs client area info
			int wBase = 40 * m_wChar;
			int hBase = 40 * m_hChar;
			DWORD style   = GetWindowLong(m_frame, GWL_STYLE);
			DWORD exStyle = GetWindowLong(m_frame, GWL_EXSTYLE);
			RECT rect;
			rect.left   = 100;
			rect.top    = 100;
			rect.right  = rect.left + wBase;
			rect.bottom = rect.top + hBase;
			AdjustWindowRectEx(&rect, style, FALSE, exStyle);
			wBase = rect.right - rect.left - wBase;
			hBase = rect.bottom - rect.top - hBase;

			// get closest size that's a multiple of the character size
			RECT* newRect = (RECT*)lParam;
			int width  = (newRect->right - newRect->left - wBase) / m_wChar;
			int height = (newRect->bottom - newRect->top - hBase) / m_hChar;
			width  = width  * m_wChar + wBase;
			height = height * m_hChar + hBase;

			// adjust sizing rect
			switch (wParam) {
			case WMSZ_LEFT:
			case WMSZ_TOPLEFT:
			case WMSZ_BOTTOMLEFT:
				newRect->left = newRect->right - width;
				break;

			case WMSZ_RIGHT:
			case WMSZ_TOPRIGHT:
			case WMSZ_BOTTOMRIGHT:
				newRect->right = newRect->left + width;
				break;
			}
			switch (wParam) {
			case WMSZ_TOP:
			case WMSZ_TOPLEFT:
			case WMSZ_TOPRIGHT:
				newRect->top = newRect->bottom - height;
				break;

			case WMSZ_BOTTOM:
			case WMSZ_BOTTOMLEFT:
			case WMSZ_BOTTOMRIGHT:
				newRect->bottom = newRect->top + height;
				break;
			}
			return TRUE;
		}
		break;

	default:
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK
CArchConsoleWindows::staticWndProc(HWND hwnd, UINT msg,
				WPARAM wParam, LPARAM lParam)
{
	// forward the message
	if (s_instance != NULL) {
		return s_instance->wndProc(hwnd, msg, wParam, lParam);
	}
	else {
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

void
CArchConsoleWindows::threadMainLoop()
{
	LoadLibrary("RICHED32.DLL");

	// get the app icons
	HICON largeIcon, smallIcon;
	CArchMiscWindows::getIcons(largeIcon, smallIcon);

	// register a window class
	WNDCLASSEX classInfo;
	classInfo.cbSize        = sizeof(classInfo);
	classInfo.style         = 0;
	classInfo.lpfnWndProc   = &CArchConsoleWindows::staticWndProc;
	classInfo.cbClsExtra    = 0;
	classInfo.cbWndExtra    = sizeof(CArchConsoleWindows*);
	classInfo.hInstance     = s_appInstance;
	classInfo.hIcon         = largeIcon;
	classInfo.hCursor       = NULL;
	classInfo.hbrBackground = NULL;
	classInfo.lpszMenuName  = NULL;
	classInfo.lpszClassName = TEXT("SynergyConsole");
	classInfo.hIconSm       = smallIcon;
	ATOM windowClass        = RegisterClassEx(&classInfo);

	// create frame window
	m_frame = CreateWindowEx(0,
							reinterpret_cast<LPCTSTR>(windowClass),
							TEXT("Synergy Log"),
							WS_OVERLAPPEDWINDOW,
							CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
							NULL,
							NULL,
							s_appInstance,
							NULL);

	// create log window
	m_hwnd = CreateWindowEx(0,
							"RichEdit",
							TEXT(""),
							WS_CHILD | WS_VISIBLE | WS_VSCROLL |
								ES_MULTILINE | ES_READONLY,
							0, 0, 1, 1,
							m_frame,
							(HMENU)1,
							s_appInstance,
							NULL);

	// select font and get info
	HDC hdc         = GetDC(m_hwnd);
	HGDIOBJ oldFont = SelectObject(hdc, GetStockObject(ANSI_FIXED_FONT));
	TEXTMETRIC metrics;
	GetTextMetrics(hdc, &metrics);
	CHARFORMAT format;
	format.cbSize          = sizeof(format);
	format.dwMask          = CFM_CHARSET | CFM_COLOR | CFM_FACE |
								CFM_OFFSET | CFM_SIZE | CFM_PROTECTED |
								CFM_BOLD | CFM_ITALIC |
								CFM_STRIKEOUT | CFM_UNDERLINE;
	format.dwEffects       = 0;
	format.yHeight         = metrics.tmHeight * TWIPS_PER_POINT; // this is in 1/1440 in (twips)
	format.yOffset         = 0;
	format.crTextColor     = RGB(0, 0, 0);
	format.bCharSet        = DEFAULT_CHARSET;
	format.bPitchAndFamily = FIXED_PITCH | FF_MODERN;
	GetTextFace(hdc, sizeof(format.szFaceName), format.szFaceName);
	SelectObject(hdc, oldFont);
	ReleaseDC(m_hwnd, hdc);

	// prep window
	SendMessage(m_hwnd, EM_EXLIMITTEXT, 0, m_maxCharacters);
	SendMessage(m_hwnd, EM_SETCHARFORMAT, 0, reinterpret_cast<LPARAM>(&format));
	SendMessage(m_hwnd, EM_SETBKGNDCOLOR, 0, RGB(255, 255, 255));
	m_wChar = metrics.tmAveCharWidth;
	m_hChar = metrics.tmHeight + metrics.tmExternalLeading;
	setSize(80, 25);

	// signal ready
	ARCH->lockMutex(m_mutex);
	m_ready = true;
	ARCH->broadcastCondVar(m_condVar);
	ARCH->unlockMutex(m_mutex);

	// handle failure
	if (m_hwnd == NULL) {
		UnregisterClass(reinterpret_cast<LPCTSTR>(windowClass), s_appInstance);
		return;
	}

	// main loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// clean up
	DestroyWindow(m_hwnd);
	UnregisterClass(reinterpret_cast<LPCTSTR>(windowClass), s_appInstance);
}

void*
CArchConsoleWindows::threadEntry(void* self)
{
	reinterpret_cast<CArchConsoleWindows*>(self)->threadMainLoop();
	return NULL;
}
