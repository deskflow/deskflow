#ifndef CMSWINDOWSSCREEN_H
#define CMSWINDOWSSCREEN_H

#include "IClipboard.h"
#include "CMutex.h"
#include "CString.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class CMSWindowsScreenSaver;
class CThread;

class CEvent {
public:
	MSG					m_msg;
	LRESULT				m_result;
};

class CMSWindowsScreen {
public:
	CMSWindowsScreen();
	virtual ~CMSWindowsScreen();

	// manipulators

	static void			init(HINSTANCE);

	// accessors

	// get the application instance handle
	static HINSTANCE	getInstance();

protected:
	// runs an event loop and returns when exitMainLoop() is called
	void				mainLoop();

	// force mainLoop() to return
	void				exitMainLoop();

	// open the X display.  calls onOpenDisplay() after opening the display,
	// getting the screen, its size, and root window.  then it starts the
	// event thread.
	void				openDisplay();

	// destroy the window and close the display.  calls onCloseDisplay()
	// after the event thread has been shut down but before the display
	// is closed.
	void				closeDisplay();

	// get the registered window class atom
	ATOM				getClass() const;

	// update screen size cache
	void				updateScreenShape();

	// get the size of the screen
	void				getScreenShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;

	// get the current cursor position
	void				getCursorPos(SInt32& x, SInt32& y) const;

	// get the cursor center position
	void				getCursorCenter(SInt32& x, SInt32& y) const;

	// get the input desktop.  caller must CloseDesktop() the result.
	// do not call under windows 95/98/me.
	HDESK				openInputDesktop() const;

	// get the desktop's name.  do not call under windows 95/98/me.
	CString				getDesktopName(HDESK) const;

	// returns true iff desk is the current desk.  do not call under
	// windows 95/98/me.
	bool				isCurrentDesktop(HDESK desk) const;

	// get the screen saver object
	CMSWindowsScreenSaver*
						getScreenSaver() const;

	// called for each event before event translation and dispatch.  return
	// true to skip translation and dispatch.  subclasses should call the
	// superclass's version first and return true if it returns true.
	virtual bool		onPreDispatch(const CEvent* event);

	// called by mainLoop().  iff the event was handled return true and
	// store the result, if any, in m_result, which defaults to zero.
	virtual bool		onEvent(CEvent* event) = 0;

	// called by isCurrentDesktop() to get the current desktop name
	virtual CString		getCurrentDesktopName() const = 0;

private:
	// create the transparent cursor
	void				createBlankCursor();

	// our window proc
	static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

private:
	static HINSTANCE	s_instance;
	ATOM				m_class;
	HICON				m_icon;
	HCURSOR				m_cursor;
	SInt32				m_x, m_y;
	SInt32				m_w, m_h;
	DWORD				m_thread;
	CMSWindowsScreenSaver*	m_screenSaver;
	static CMSWindowsScreen*	s_screen;
};

#endif
