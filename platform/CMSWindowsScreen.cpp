#include "CMSWindowsScreen.h"
#include "CThread.h"
#include "CLock.h"
#include "TMethodJob.h"
#include "CLog.h"
#include "CString.h"
#include <string.h>
#include <assert.h>

//
// CMSWindowsScreen
//

HINSTANCE				CMSWindowsScreen::s_instance = NULL;
CMSWindowsScreen*		CMSWindowsScreen::s_screen = NULL;

CMSWindowsScreen::CMSWindowsScreen() :
								m_class(0),
								m_cursor(NULL),
								m_w(0), m_h(0),
								m_thread(0)
{
	assert(s_screen == NULL);
	s_screen = this;
}

CMSWindowsScreen::~CMSWindowsScreen()
{
	assert(m_class == 0);
	s_screen = NULL;
}

void					CMSWindowsScreen::init(HINSTANCE instance)
{
	s_instance = instance;
}

void					CMSWindowsScreen::doRun()
{
	// save thread id for posting quit message
	m_thread = GetCurrentThreadId();

	// event loop
	for (;;) {
		// wait for and get the next event
		MSG msg;
		getEvent(&msg);

		// handle quit message
		if (msg.message == WM_QUIT) {
			break;
		}

		// dispatch message
		if (!onPreTranslate(&msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void					CMSWindowsScreen::doStop()
{
	PostThreadMessage(m_thread, WM_QUIT, 0, 0);
}

void					CMSWindowsScreen::openDisplay()
{
	assert(s_instance != NULL);
	assert(m_class    == 0);

	// create a transparent cursor
	int cw = GetSystemMetrics(SM_CXCURSOR);
	int ch = GetSystemMetrics(SM_CYCURSOR);
	UInt8* cursorAND = new UInt8[ch * ((cw + 31) >> 2)];
	UInt8* cursorXOR = new UInt8[ch * ((cw + 31) >> 2)];
	memset(cursorAND, 0xff, ch * ((cw + 31) >> 2));
	memset(cursorXOR, 0x00, ch * ((cw + 31) >> 2));
	m_cursor = CreateCursor(s_instance, 0, 0, cw, ch, cursorAND, cursorXOR);
	delete[] cursorXOR;
	delete[] cursorAND;

	// register a window class
	WNDCLASSEX classInfo;
	classInfo.cbSize        = sizeof(classInfo);
	classInfo.style         = CS_DBLCLKS | CS_NOCLOSE;
	classInfo.lpfnWndProc   = &CMSWindowsScreen::wndProc;
	classInfo.cbClsExtra    = 0;
	classInfo.cbWndExtra    = 0;
	classInfo.hInstance     = s_instance;
	classInfo.hIcon         = NULL;
	classInfo.hCursor       = m_cursor;
	classInfo.hbrBackground = NULL;
	classInfo.lpszMenuName  = NULL;
	classInfo.lpszClassName = "Synergy";
	classInfo.hIconSm       = NULL;
	m_class = RegisterClassEx(&classInfo);

	// get screen size
	// FIXME -- should handle multiple screens
	m_w = GetSystemMetrics(SM_CXSCREEN);
	m_h = GetSystemMetrics(SM_CYSCREEN);
	log((CLOG_INFO "display size: %dx%d", m_w, m_h));

	// let subclass prep display
	onOpenDisplay();
}

void					CMSWindowsScreen::closeDisplay()
{
	assert(s_instance    != NULL);
	assert(m_class       != 0);

	// let subclass close down display
	onCloseDisplay();

	// unregister the window class
	UnregisterClass((LPCTSTR)m_class, s_instance);
	m_class = 0;

	// delete resources
	DestroyCursor(m_cursor);
	m_cursor = NULL;

	log((CLOG_DEBUG "closed display"));
}

HINSTANCE				CMSWindowsScreen::getInstance()
{
	return s_instance;
}

ATOM					CMSWindowsScreen::getClass() const
{
	return m_class;
}

void					CMSWindowsScreen::updateScreenSize()
{
	m_w = GetSystemMetrics(SM_CXSCREEN);
	m_h = GetSystemMetrics(SM_CYSCREEN);
	log((CLOG_INFO "display resize: %dx%d", m_w, m_h));
}

void					CMSWindowsScreen::getScreenSize(
								SInt32* w, SInt32* h) const
{
	assert(m_class != 0);
	assert(w != NULL && h != NULL);

	*w = m_w;
	*h = m_h;
}

HDESK					CMSWindowsScreen::openInputDesktop() const
{
	return OpenInputDesktop(DF_ALLOWOTHERACCOUNTHOOK, TRUE,
								DESKTOP_CREATEWINDOW |
									DESKTOP_HOOKCONTROL |
									GENERIC_WRITE);
}

CString					CMSWindowsScreen::getDesktopName(
								HDESK desk) const
{
	if (desk == NULL) {
		return CString();
	}
	else {
		DWORD size;
		GetUserObjectInformation(desk, UOI_NAME, NULL, 0, &size);
		TCHAR* name = new TCHAR[size / sizeof(TCHAR) + 1];
		GetUserObjectInformation(desk, UOI_NAME, name, size, &size);
		CString result(name);
		delete[] name;
		return result;
	}
}

bool					CMSWindowsScreen::isCurrentDesktop(
								HDESK desk) const
{
	return CStringUtil::CaselessCmp::equal(getDesktopName(desk),
								getCurrentDesktopName());
}

void					CMSWindowsScreen::getEvent(MSG* msg) const
{
	// wait for an event in a cancellable way
	while (HIWORD(GetQueueStatus(QS_ALLINPUT)) == 0) {
		CThread::sleep(0.01);
	}
	GetMessage(msg, NULL, 0, 0);
}

LRESULT CALLBACK		CMSWindowsScreen::wndProc(
								HWND hwnd, UINT msg,
								WPARAM wParam, LPARAM lParam)
{
	assert(s_screen != NULL);
	return s_screen->onEvent(hwnd, msg, wParam, lParam);
}
