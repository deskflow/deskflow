#ifndef CMSWINDOWSSCREEN_H
#define CMSWINDOWSSCREEN_H

#include "CMutex.h"
#include "IClipboard.h"
#include "BasicTypes.h"
#include <windows.h>

class CString;
class CThread;

class CMSWindowsScreen {
public:
	CMSWindowsScreen();
	virtual ~CMSWindowsScreen();

	// manipulators

	static void			init(HINSTANCE);

protected:
	// runs an event loop and returns when WM_QUIT is received
	void				doRun();

	// sends WM_QUIT to force doRun() to return
	void				doStop();

	// open the X display.  calls onOpenDisplay() after opening the display,
	// getting the screen, its size, and root window.  then it starts the
	// event thread.
	void				openDisplay();

	// destroy the window and close the display.  calls onCloseDisplay()
	// after the event thread has been shut down but before the display
	// is closed.
	void				closeDisplay();

	// get the application instance handle and the registered window
	// class atom
	static HINSTANCE	getInstance();
	ATOM				getClass() const;

	// get the size of the screen
	void				getScreenSize(SInt32* w, SInt32* h) const;

	// wait for and get the next message.  cancellable.
	void				getEvent(MSG*) const;

	// called by doRun() to handle an event.  return true to skip
	// event translation and dispatch.
	virtual bool		onPreTranslate(MSG*) = 0;

	// called by window proc.  subclass must call DefWindowProc() if necessary
	virtual LRESULT		onEvent(HWND, UINT, WPARAM, LPARAM) = 0;

	// called by openDisplay() to allow subclasses to prepare the display
	virtual void		onOpenDisplay() = 0;

	// called by closeDisplay() to 
	virtual void		onCloseDisplay() = 0;

private:
	static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

private:
	static HINSTANCE	s_instance;
	ATOM				m_class;
	HICON				m_icon;
	HCURSOR				m_cursor;
	SInt32				m_w, m_h;
	DWORD				m_thread;
	static CMSWindowsScreen*	s_screen;
};

#endif
