#include "CMSWindowsScreen.h"
#include "CMSWindowsClipboard.h"
#include "CMSWindowsScreenSaver.h"
#include "CPlatform.h"
#include "CClipboard.h"
#include "IMSWindowsScreenEventHandler.h"
#include "IScreenReceiver.h"
#include "XSynergy.h"
#include "CThread.h"
#include "CLock.h"
#include "TMethodJob.h"
#include "CLog.h"
#include "CString.h"
#include <cstring>

//
// add backwards compatible multihead support (and suppress bogus warning)
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

CMSWindowsScreen::CMSWindowsScreen(IScreenReceiver* receiver,
				IMSWindowsScreenEventHandler* eventHandler) :
	m_receiver(receiver),
	m_eventHandler(eventHandler),
	m_class(NULL),
	m_icon(NULL),
	m_cursor(NULL),
	m_is95Family(CPlatform::isWindows95Family()),
	m_window(NULL),
	m_x(0), m_y(0),
	m_w(0), m_h(0),
	m_multimon(false),
	m_threadID(0),
	m_lastThreadID(0),
	m_nextClipboardWindow(NULL),
	m_clipboardOwner(NULL),
	m_timer(0),
	m_desk(NULL),
	m_deskName(),
	m_hookLibrary(NULL),
	m_installScreensaver(NULL),
	m_uninstallScreensaver(NULL),
	m_screensaver(NULL),
	m_screensaverNotify(false)
{
	assert(s_screen       == NULL);
	assert(m_receiver     != NULL);
	assert(m_eventHandler != NULL);

	s_screen = this;

	// make sure this thread has a message queue
	MSG dummy;
	PeekMessage(&dummy, NULL, WM_USER, WM_USER, PM_NOREMOVE);
}

CMSWindowsScreen::~CMSWindowsScreen()
{
	assert(s_screen != NULL);
	assert(m_class  == 0);

	s_screen = NULL;
}

void
CMSWindowsScreen::init(HINSTANCE instance)
{
	s_instance = instance;
}

HWND
CMSWindowsScreen::openDesktop()
{
	// save thread id
	m_threadID = GetCurrentThreadId();

	// get the input desktop and switch to it
	if (!switchDesktop(openInputDesktop())) {
		return NULL;
	}

	// poll input desktop to see if it changes (onPreDispatch()
	// handles WM_TIMER)
	m_timer = 0;
	if (!m_is95Family) {
		m_timer = SetTimer(NULL, 0, 200, NULL);
	}

	return m_window;
}

void
CMSWindowsScreen::closeDesktop()
{
	// remove timer
	if (m_timer != 0) {
		KillTimer(NULL, m_timer);
	}

	// disconnect from desktop
	switchDesktop(NULL);

	// clear thread id
	m_threadID = 0;

	assert(m_window == NULL);
	assert(m_desk   == NULL);
}

bool
CMSWindowsScreen::isMultimon() const
{
	return m_multimon;
}

HINSTANCE
CMSWindowsScreen::getInstance()
{
	return s_instance;
}

void
CMSWindowsScreen::open()
{
	assert(s_instance != NULL);
	assert(m_class    == 0);

	log((CLOG_DEBUG "opening display"));

	// create the transparent cursor
	createBlankCursor();

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
	m_class                 = RegisterClassEx(&classInfo);

	// get screen shape
	updateScreenShape();

	// initialize the screen saver
	m_screensaver = new CMSWindowsScreenSaver();

	// load the hook library and get the screen saver functions
	m_hookLibrary = LoadLibrary("synrgyhk");
	if (m_hookLibrary != NULL) {
		m_installScreensaver   = (InstallScreenSaverFunc)GetProcAddress(
									m_hookLibrary, "installScreenSaver");
		m_uninstallScreensaver = (UninstallScreenSaverFunc)GetProcAddress(
									m_hookLibrary, "uninstallScreenSaver");
		if (m_installScreensaver == NULL || m_uninstallScreensaver == NULL) {
			// disable if either install or uninstall is unavailable
			m_installScreensaver   = NULL;
			m_uninstallScreensaver = NULL;
		}
	}
}

void
CMSWindowsScreen::mainLoop()
{
	// must call mainLoop() from same thread as openDesktop()
	assert(m_threadID == GetCurrentThreadId());

	// event loop
	CEvent event;
	event.m_result = 0;
	for (;;) {
		// wait for an event in a cancellable way
		CThread::waitForEvent();
		GetMessage(&event.m_msg, NULL, 0, 0);

		// handle quit message
		if (event.m_msg.message == WM_QUIT) {
			break;
		}

		// dispatch message
		if (!onPreDispatch(&event)) {
			TranslateMessage(&event.m_msg);
			DispatchMessage(&event.m_msg);
		}
	}
}

void
CMSWindowsScreen::exitMainLoop()
{
	PostThreadMessage(m_threadID, WM_QUIT, 0, 0);
}

void
CMSWindowsScreen::close()
{
	assert(s_instance != NULL);

	// done with hook library
	if (m_hookLibrary != NULL) {
		FreeLibrary(m_hookLibrary);
		m_installScreensaver   = NULL;
		m_uninstallScreensaver = NULL;
		m_hookLibrary          = NULL;
	}

	// done with screen saver
	delete m_screensaver;
	m_screensaver = NULL;

	// unregister the window class
	if (m_class != 0) {
		UnregisterClass((LPCTSTR)m_class, s_instance);
		m_class = 0;
	}

	// delete resources
	if (m_cursor != NULL) {
		DestroyCursor(m_cursor);
		m_cursor = NULL;
	}

	log((CLOG_DEBUG "closed display"));
}

bool
CMSWindowsScreen::setClipboard(ClipboardID, const IClipboard* src)
{
	CMSWindowsClipboard dst(m_window);
	if (src != NULL) {
		// save clipboard data
		return CClipboard::copy(&dst, src);
	}
	else {
		// assert clipboard ownership
		if (!dst.open(0)) {
			return false;
		}
		dst.empty();
		dst.close();
		return true;
	}
}

void
CMSWindowsScreen::checkClipboards()
{
	// if we think we own the clipboard but we don't then somebody
	// grabbed the clipboard on this screen without us knowing.
	// tell the server that this screen grabbed the clipboard.
	//
	// this works around bugs in the clipboard viewer chain.
	// sometimes NT will simply never send WM_DRAWCLIPBOARD
	// messages for no apparent reason and rebooting fixes the
	// problem.  since we don't want a broken clipboard until the
	// next reboot we do this double check.  clipboard ownership
	// won't be reflected on other screens until we leave but at
	// least the clipboard itself will work.
	HWND clipboardOwner = GetClipboardOwner();
	if (m_clipboardOwner != clipboardOwner) {
		try {
			m_clipboardOwner = clipboardOwner;
			if (m_clipboardOwner != m_window && m_clipboardOwner != NULL) {
				m_receiver->onGrabClipboard(kClipboardClipboard);
				m_receiver->onGrabClipboard(kClipboardSelection);
			}
		}
		catch (XBadClient&) {
			// ignore
		}
	}
}

void
CMSWindowsScreen::openScreensaver(bool notify)
{
	assert(m_screensaver != NULL);

	m_screensaverNotify = notify;
	if (m_screensaverNotify) {
		if (m_installScreensaver != NULL) {
			m_installScreensaver();
		}
	}
	else {
		m_screensaver->disable();
	}
}

void
CMSWindowsScreen::closeScreensaver()
{
	if (m_screensaver != NULL) {
		if (m_screensaverNotify) {
			if (m_uninstallScreensaver != NULL) {
				m_uninstallScreensaver();
			}
		}
		else {
			m_screensaver->enable();
		}
	}
	m_screensaverNotify = false;
}

void
CMSWindowsScreen::screensaver(bool activate)
{
	assert(m_screensaver != NULL);
	if (activate) {
		m_screensaver->activate();
	}
	else {
		m_screensaver->deactivate();
	}
}

void
CMSWindowsScreen::syncDesktop()
{
	// change calling thread's desktop
	if (!m_is95Family) {
		if (SetThreadDesktop(m_desk) == 0) {
			log((CLOG_WARN "failed to set desktop: %d", GetLastError()));
		}
	}

	// attach input queues if not already attached.  this has a habit
	// of sucking up more and more CPU each time it's called (even if
	// the threads are already attached).  since we only expect one
	// thread to call this more than once we can save just the last
	// the attached thread.
	DWORD threadID = GetCurrentThreadId();
	if (threadID != m_lastThreadID && threadID != m_threadID) {
		m_lastThreadID = threadID;
		AttachThreadInput(threadID, m_threadID, TRUE);
	}
}

bool
CMSWindowsScreen::getClipboard(ClipboardID, IClipboard* dst) const
{
	CMSWindowsClipboard src(m_window);
	CClipboard::copy(dst, &src);
	return true;
}

void
CMSWindowsScreen::getShape(SInt32& x, SInt32& y,
				SInt32& w, SInt32& h) const
{
	assert(m_class != 0);

	x = m_x;
	y = m_y;
	w = m_w;
	h = m_h;
}

void
CMSWindowsScreen::getCursorPos(SInt32& x, SInt32& y) const
{
	POINT pos;
	if (GetCursorPos(&pos)) {
		x = pos.x;
		y = pos.y;
	}
	else {
		getCursorCenter(x, y);
	}
}

void
CMSWindowsScreen::getCursorCenter(SInt32& x, SInt32& y) const
{
	x = GetSystemMetrics(SM_CXSCREEN) >> 1;
	y = GetSystemMetrics(SM_CYSCREEN) >> 1;
}

void
CMSWindowsScreen::updateScreenShape()
{
	// get shape
	m_x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	m_y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	m_w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	m_h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	log((CLOG_INFO "screen shape: %d,%d %dx%d", m_x, m_y, m_w, m_h));

	// check for multiple monitors
	m_multimon = (m_w != GetSystemMetrics(SM_CXSCREEN) ||
				  m_h != GetSystemMetrics(SM_CYSCREEN));
}

bool
CMSWindowsScreen::onPreDispatch(const CEvent* event)
{
	// handle event
	const MSG* msg = &event->m_msg;
	switch (msg->message) {
	case SYNERGY_MSG_SCREEN_SAVER:
		{
			// activating or deactivating?
			bool activate = (msg->wParam != 0);

			// ignore this message if there are any other screen saver
			// messages already in the queue.  this is important because
			// our checkStarted() function has a deliberate delay, so it
			// can't respond to events at full CPU speed and will fall
			// behind if a lot of screen saver events are generated.
			// that can easily happen because windows will continually
			// send SC_SCREENSAVE until the screen saver starts, even if
			// the screen saver is disabled!
			MSG msg;
			if (!PeekMessage(&msg, NULL, SYNERGY_MSG_SCREEN_SAVER,
								SYNERGY_MSG_SCREEN_SAVER, PM_NOREMOVE)) {
				if (activate) {
					if (m_screensaver->checkStarted(
									SYNERGY_MSG_SCREEN_SAVER, FALSE, 0)) {
						m_eventHandler->onScreensaver(true);
					}
				}
				else {
					m_eventHandler->onScreensaver(false);
				}
			}
			return true;
		}

	case WM_TIMER:
		// if current desktop is not the input desktop then switch to it.
		// windows 95 doesn't support multiple desktops so don't bother
		// to check under it.
		if (!m_is95Family) {
			HDESK desk = openInputDesktop();
			if (desk != NULL) {
				if (isCurrentDesktop(desk)) {
					CloseDesktop(desk);
				}
				else {
					switchDesktop(desk);
				}
			}
		}
		return true;
	}

	return m_eventHandler->onPreDispatch(event);
}

bool
CMSWindowsScreen::onEvent(CEvent* event)
{
	assert(event != NULL);

	const MSG& msg = event->m_msg;
	switch (msg.message) {
	case WM_QUERYENDSESSION:
		if (m_is95Family) {
			event->m_result = TRUE;
			return true;
		}
		break;

	case WM_ENDSESSION:
		if (m_is95Family) {
			if (msg.wParam == TRUE && msg.lParam == 0) {
				exitMainLoop();
			}
			return true;
		}
		break;

	case WM_PAINT:
		ValidateRect(msg.hwnd, NULL);
		return true;

	case WM_DRAWCLIPBOARD:
		log((CLOG_DEBUG "clipboard was taken"));

		// first pass it on
		if (m_nextClipboardWindow != NULL) {
			SendMessage(m_nextClipboardWindow,
								msg.message, msg.wParam, msg.lParam);
		}

		// now notify client that somebody changed the clipboard (unless
		// we're now the owner, in which case it's because we took
		// ownership, or now it's owned by nobody, which will happen if
		// we owned it and switched desktops because we destroy our
		// window to do that).
		try {
			m_clipboardOwner = GetClipboardOwner();
			if (m_clipboardOwner != m_window && m_clipboardOwner != NULL) {
				m_receiver->onGrabClipboard(kClipboardClipboard);
				m_receiver->onGrabClipboard(kClipboardSelection);
			}
		}
		catch (XBadClient&) {
			// ignore.  this can happen if we receive this event
			// before we've fully started up.
		}
		return true;

	case WM_CHANGECBCHAIN:
		if (m_nextClipboardWindow == (HWND)msg.wParam) {
			m_nextClipboardWindow = (HWND)msg.lParam;
		}
		else if (m_nextClipboardWindow != NULL) {
			SendMessage(m_nextClipboardWindow,
								msg.message, msg.wParam, msg.lParam);
		}
		return true;

	case WM_DISPLAYCHANGE:
		{
			// screen resolution may have changed.  get old shape.
			SInt32 xOld, yOld, wOld, hOld;
			getShape(xOld, yOld, wOld, hOld);

			// update shape
			updateScreenShape();

			// collect new screen info
			CClientInfo info;
			getShape(info.m_x, info.m_y, info.m_w, info.m_h);
			getCursorPos(info.m_mx, info.m_my);
			info.m_zoneSize = m_eventHandler->getJumpZoneSize();

			// do nothing if resolution hasn't changed
			if (info.m_x != xOld || info.m_y != yOld ||
				info.m_w != wOld || info.m_h != hOld) {
				// forward event
				m_eventHandler->onEvent(event);

				// send new screen info
				m_receiver->onInfoChanged(info);
			}

			return true;
		}
	}

	return m_eventHandler->onEvent(event);
}

void
CMSWindowsScreen::createBlankCursor()
{
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
}

bool
CMSWindowsScreen::switchDesktop(HDESK desk)
{
	// did we own the clipboard?
	bool ownClipboard = (m_clipboardOwner == m_window && m_window != NULL);

	// destroy old window
	if (m_window != NULL) {
		// first remove clipboard snooper
		ChangeClipboardChain(m_window, m_nextClipboardWindow);
		m_nextClipboardWindow = NULL;

		// we no longer own the clipboard
		if (ownClipboard) {
			m_clipboardOwner = NULL;
		}

		// let client clean up before we destroy the window
		m_eventHandler->preDestroyWindow(m_window);

		// now destroy window
		DestroyWindow(m_window);
		m_window = NULL;

		// done with desk
		if (!m_is95Family) {
			CloseDesktop(m_desk);
		}
		m_desk     = NULL;
		m_deskName = "";
	}

	// if no new desktop then we're done
	if (desk == NULL) {
		log((CLOG_INFO "disconnecting desktop"));
		return true;
	}

	// uninstall screen saver hooks
	if (m_screensaverNotify) {
		if (m_uninstallScreensaver != NULL) {
			m_uninstallScreensaver();
		}
	}

	// set the desktop.  can only do this when there are no windows
	// and hooks on the current desktop owned by this thread.
	if (SetThreadDesktop(desk) == 0) {
		log((CLOG_ERR "failed to set desktop: %d", GetLastError()));
		if (!m_is95Family) {
			CloseDesktop(desk);
		}
		return false;
	}

	// create the window
	m_window = CreateWindowEx(WS_EX_TOPMOST |
									WS_EX_TRANSPARENT |
									WS_EX_TOOLWINDOW,
								(LPCTSTR)m_class,
								"Synergy",
								WS_POPUP,
								0, 0, 1, 1,
								NULL, NULL,
								getInstance(),
								NULL);
	if (m_window == NULL) {
		log((CLOG_ERR "failed to create window: %d", GetLastError()));
		if (!m_is95Family) {
			CloseDesktop(desk);
		}
		return false;
	}

	// reinstall screen saver hooks
	if (m_screensaverNotify) {
		if (m_installScreensaver != NULL) {
			m_installScreensaver();
		}
	}

	// install our clipboard snooper
	m_nextClipboardWindow = SetClipboardViewer(m_window);

	// reassert clipboard ownership
	if (ownClipboard) {
		// FIXME -- take clipboard ownership, but we should also set
		// the clipboard data.
	}
	m_clipboardOwner = GetClipboardOwner();

	// save new desktop
	m_desk     = desk;
	m_deskName = getDesktopName(m_desk);
	log((CLOG_INFO "switched to desktop \"%s\"", m_deskName.c_str()));

	// let client prepare the window
	m_eventHandler->postCreateWindow(m_window);

	return true;
}

HDESK
CMSWindowsScreen::openInputDesktop() const
{
	if (m_is95Family) {
		// there's only one desktop on windows 95 et al.
		return GetThreadDesktop(GetCurrentThreadId());
	}
	else {
		return OpenInputDesktop(DF_ALLOWOTHERACCOUNTHOOK, TRUE,
								DESKTOP_CREATEWINDOW |
									DESKTOP_HOOKCONTROL |
									GENERIC_WRITE);
	}
}

CString
CMSWindowsScreen::getDesktopName(HDESK desk) const
{
	if (desk == NULL) {
		return CString();
	}
	else if (m_is95Family) {
		return "desktop";
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
	return CStringUtil::CaselessCmp::equal(getDesktopName(desk), m_deskName);
}

LRESULT CALLBACK
CMSWindowsScreen::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	assert(s_screen != NULL);

	CEvent event;
	event.m_msg.hwnd    = hwnd;
	event.m_msg.message = msg;
	event.m_msg.wParam  = wParam;
	event.m_msg.lParam  = lParam;
	event.m_result      = 0;

	if (s_screen->onEvent(&event)) {
		return event.m_result;
	}
	else {
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}
