#ifndef CXWINDOWSSCREEN_H
#define CXWINDOWSSCREEN_H

#include "CClipboard.h"
#include "CMutex.h"
#include "BasicTypes.h"
#include <X11/Xlib.h>

class CString;

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
	bool				getEvent(XEvent*) const;

	// cause getEvent() to return false immediately and forever after
	void				doStop();

	bool				setDisplayClipboard(const IClipboard* clipboard,
								Window requestor, Time timestamp);

	// copy the clipboard contents to clipboard.  requestor must be a
	// valid window;  it will be used to receive the transfer.  timestamp
	// should be the timestamp of the provoking event and not CurrentTime.
	void				getDisplayClipboard(IClipboard* clipboard,
								Window requestor, Time timestamp) const;

	// add a selection request to the request list
	void				addClipboardRequest(
								Window owner, Window requestor,
								Atom selection, Atom target,
								Atom property, Time time);

	// continue processing a selection request
	void				processClipboardRequest(Window window,
								Atom property, Time time);

	// called by openDisplay() to allow subclasses to prepare the display
	virtual void		onOpenDisplay() = 0;

	// called by closeDisplay() to 
	virtual void		onCloseDisplay() = 0;

  private:
	struct PropertyNotifyInfo {
	  public:
		Window			m_window;
		Atom			m_property;
	};

	bool				getDisplayClipboard(Atom selection, Atom type,
								Window requestor, Time timestamp,
								Atom* outputType, CString* data) const;
	bool				getData(Window, Atom property,
								Atom* type, SInt32* datumSize,
								CString* data) const;
	IClipboard::EFormat	getFormat(Atom) const;
	static Bool			findSelectionNotify(Display*,
								XEvent* xevent, XPointer arg);
	static Bool			findPropertyNotify(Display*,
								XEvent* xevent, XPointer arg);

	void				sendClipboardTargetsNotify(Window requestor,
								Atom property, Time time);
	void				addClipboardRequest(Window requestor,
								Atom property, Time time,
								IClipboard::EFormat);
	void				addClipboardMultipleRequest(Window requestor,
								Atom property, Time time);

  private:
	Display*			m_display;
	int					m_screen;
	Window				m_root;
	SInt32				m_w, m_h;
	bool				m_stop;

	// atoms we'll need
	Atom				m_atomTargets;
	Atom				m_atomMultiple;
	Atom				m_atomData;
	Atom				m_atomINCR;
	Atom				m_atomText;
	Atom				m_atomCompoundText;

	// the contents of our selection
	CClipboard			m_clipboard;

	// X is not thread safe
	CMutex				m_mutex;
};

#endif
