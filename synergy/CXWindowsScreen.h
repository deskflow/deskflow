#ifndef CXWINDOWSSCREEN_H
#define CXWINDOWSSCREEN_H

#include "CMutex.h"
#include "BasicTypes.h"
#include <X11/Xlib.h>

class CThread;

class CXWindowsScreen {
  public:
	CXWindowsScreen();
	virtual ~CXWindowsScreen();

  protected:
	class CDisplayLock {
	  public:
		CDisplayLock(const CXWindowsScreen*);
		~CDisplayLock();

		operator Display*() const;

	  private:
		const CMutex*	m_mutex;
		Display*		m_display;
	};
	friend class CDisplayLock;

	// open the X display.  calls onOpenDisplay() after opening the display,
	// getting the screen, its size, and root window.  then it starts the
	// event thread.
	void				openDisplay();

	// destroy the window and close the display.  calls onCloseDisplay()
	// after the event thread has been shut down but before the display
	// is closed.
	void				closeDisplay();

	// get the opened screen, its size, its root window.  to get the
	// display create a CDisplayLock object passing this.  while the
	// object exists no other threads may access the display.  do not
	// save the Display* beyond the lifetime of the CDisplayLock.
	int					getScreen() const;
	void				getScreenSize(SInt32* w, SInt32* h) const;
	Window				getRoot() const;

	// create a cursor that is transparent everywhere
	Cursor				createBlankCursor() const;

	// wait for and get the next X event.  cancellable.
	void				getEvent(XEvent*) const;

	// called by openDisplay() to allow subclasses to prepare the display
	virtual void		onOpenDisplay() = 0;

	// called by closeDisplay() to 
	virtual void		onCloseDisplay() = 0;

	// override to process X events
	virtual void		eventThread(void*) = 0;

  private:
	CThread*			m_eventThread;
	Display*			m_display;
	int					m_screen;
	Window				m_root;
	SInt32				m_w, m_h;

	// X is not thread safe
	CMutex				m_mutex;
};

#endif
