#include "CMSWindowsScreen.h"
#include "CMSWindowsScreenSaver.h"
#include "CThread.h"
#include "CLock.h"
#include "TMethodJob.h"
#include "CLog.h"
#include "CString.h"
#include <cstring>

//
// add backwards compatible multihead support (suppress bogus warning)
//
#pragma warning(push)
#pragma warning(disable: 4706) // assignment within conditional
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>
#pragma warning(pop)

//
// CMSWindowsScreen
//

HINSTANCE				CMSWindowsScreen::s_instance = NULL;
CMSWindowsScreen*		CMSWindowsScreen::s_screen = NULL;

CMSWindowsScreen::CMSWindowsScreen() :
	m_class(0),
	m_cursor(NULL),
	m_x(0), m_y(0),
	m_w(0), m_h(0),
	m_thread(0),
	m_screenSaver(NULL)
{
	assert(s_screen == NULL);
	s_screen = this;
}

CMSWindowsScreen::~CMSWindowsScreen()
{
	assert(m_class == 0);
	s_screen = NULL;
}

void
CMSWindowsScreen::init(HINSTANCE instance)
{
	s_instance = instance;
}

void
CMSWindowsScreen::doRun()
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

void
CMSWindowsScreen::doStop()
{
	PostThreadMessage(m_thread, WM_QUIT, 0, 0);
}

void
CMSWindowsScreen::openDisplay()
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

	// get screen shape
	updateScreenShape();

	// let subclass prep display
	onOpenDisplay();

	// initialize the screen saver
	m_screenSaver = new CMSWindowsScreenSaver();
}

void
CMSWindowsScreen::closeDisplay()
{
	assert(s_instance != NULL);
	assert(m_class    != 0);

	// done with screen saver
	delete m_screenSaver;
	m_screenSaver = NULL;

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

HINSTANCE
CMSWindowsScreen::getInstance()
{
	return s_instance;
}

ATOM
CMSWindowsScreen::getClass() const
{
	return m_class;
}

void
CMSWindowsScreen::updateScreenShape()
{
	m_x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	m_y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	m_w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	m_h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	log((CLOG_INFO "screen shape: %d,%d %dx%d", m_x, m_y, m_w, m_h));
}

void
CMSWindowsScreen::getScreenShape(
				SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	assert(m_class != 0);

	x = m_x;
	y = m_y;
	w = m_w;
	h = m_h;
}

HDESK
CMSWindowsScreen::openInputDesktop() const
{
	return OpenInputDesktop(DF_ALLOWOTHERACCOUNTHOOK, TRUE,
								DESKTOP_CREATEWINDOW |
									DESKTOP_HOOKCONTROL |
									GENERIC_WRITE);
}

CString
CMSWindowsScreen::getDesktopName(HDESK desk) const
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

bool
CMSWindowsScreen::isCurrentDesktop(HDESK desk) const
{
	return CStringUtil::CaselessCmp::equal(getDesktopName(desk),
								getCurrentDesktopName());
}

CMSWindowsScreenSaver*
CMSWindowsScreen::getScreenSaver() const
{
	return m_screenSaver;
}

void
CMSWindowsScreen::getEvent(MSG* msg) const
{
	// wait for an event in a cancellable way
	CThread::waitForEvent();
	GetMessage(msg, NULL, 0, 0);
}

LRESULT CALLBACK
CMSWindowsScreen::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	assert(s_screen != NULL);
	return s_screen->onEvent(hwnd, msg, wParam, lParam);
}
