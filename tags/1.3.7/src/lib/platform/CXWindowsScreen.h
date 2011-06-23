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

#ifndef CXWINDOWSSCREEN_H
#define CXWINDOWSSCREEN_H

#include "CPlatformScreen.h"
#include "stdset.h"
#include "stdvector.h"
#if X_DISPLAY_MISSING
#	error X11 is required to build synergy
#else
#	include <X11/Xlib.h>
#endif

class CXWindowsClipboard;
class CXWindowsKeyState;
class CXWindowsScreenSaver;

//! Implementation of IPlatformScreen for X11
class CXWindowsScreen : public CPlatformScreen {
public:
	CXWindowsScreen(const char* displayName, bool isPrimary, bool disableXInitThreads, int mouseScrollDelta=0);
	virtual ~CXWindowsScreen();

	//! @name manipulators
	//@{

	//@}

	// IScreen overrides
	virtual void*		getEventTarget() const;
	virtual bool		getClipboard(ClipboardID id, IClipboard*) const;
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;
	virtual void		getCursorPos(SInt32& x, SInt32& y) const;

	// IPrimaryScreen overrides
	virtual void		reconfigure(UInt32 activeSides);
	virtual void		warpCursor(SInt32 x, SInt32 y);
	virtual UInt32		registerHotKey(KeyID key, KeyModifierMask mask);
	virtual void		unregisterHotKey(UInt32 id);
	virtual void		fakeInputBegin();
	virtual void		fakeInputEnd();
	virtual SInt32		getJumpZoneSize() const;
	virtual bool		isAnyMouseButtonDown() const;
	virtual void		getCursorCenter(SInt32& x, SInt32& y) const;

	// ISecondaryScreen overrides
	virtual void		fakeMouseButton(ButtonID id, bool press) const;
	virtual void		fakeMouseMove(SInt32 x, SInt32 y) const;
	virtual void		fakeMouseRelativeMove(SInt32 dx, SInt32 dy) const;
	virtual void		fakeMouseWheel(SInt32 xDelta, SInt32 yDelta) const;

	// IPlatformScreen overrides
	virtual void		enable();
	virtual void		disable();
	virtual void		enter();
	virtual bool		leave();
	virtual bool		setClipboard(ClipboardID, const IClipboard*);
	virtual void		checkClipboards();
	virtual void		openScreensaver(bool notify);
	virtual void		closeScreensaver();
	virtual void		screensaver(bool activate);
	virtual void		resetOptions();
	virtual void		setOptions(const COptionsList& options);
	virtual void		setSequenceNumber(UInt32);
	virtual bool		isPrimary() const;

protected:
	// IPlatformScreen overrides
	virtual void		handleSystemEvent(const CEvent&, void*);
	virtual void		updateButtons();
	virtual IKeyState*	getKeyState() const;

private:
	// event sending
	void				sendEvent(CEvent::Type, void* = NULL);
	void				sendClipboardEvent(CEvent::Type, ClipboardID);

	// create the transparent cursor
	Cursor				createBlankCursor() const;

	// determine the clipboard from the X selection.  returns
	// kClipboardEnd if no such clipboard.
	ClipboardID			getClipboardID(Atom selection) const;

	// continue processing a selection request
	void				processClipboardRequest(Window window,
							Time time, Atom property);

	// terminate a selection request
	void				destroyClipboardRequest(Window window);

	// X I/O error handler
	void				onError();
	static int			ioErrorHandler(Display*);

private:
	class CKeyEventFilter {
	public:
		int				m_event;
		Window			m_window;
		Time			m_time;
		KeyCode			m_keycode;
	};

	Display*			openDisplay(const char* displayName);
	void				saveShape();
	Window				openWindow() const;
	void				openIM();

	bool				grabMouseAndKeyboard();
	void				onKeyPress(XKeyEvent&);
	void				onKeyRelease(XKeyEvent&, bool isRepeat);
	bool				onHotKey(XKeyEvent&, bool isRepeat);
	void				onMousePress(const XButtonEvent&);
	void				onMouseRelease(const XButtonEvent&);
	void				onMouseMove(const XMotionEvent&);

	void				selectEvents(Window) const;
	void				doSelectEvents(Window) const;

	KeyID				mapKeyFromX(XKeyEvent*) const;
	ButtonID			mapButtonFromX(const XButtonEvent*) const;
	unsigned int		mapButtonToX(ButtonID id) const;

	void				warpCursorNoFlush(SInt32 x, SInt32 y);

	void				refreshKeyboard(XEvent*);

	static Bool			findKeyEvent(Display*, XEvent* xevent, XPointer arg);

private:
	struct CHotKeyItem {
	public:
		CHotKeyItem(int, unsigned int);

		bool			operator<(const CHotKeyItem&) const;

	private:
		int				m_keycode;
		unsigned int	m_mask;
	};
	typedef std::set<bool> CFilteredKeycodes;
	typedef std::vector<std::pair<int, unsigned int> > HotKeyList;
	typedef std::map<UInt32, HotKeyList> HotKeyMap;
	typedef std::vector<UInt32> HotKeyIDList;
	typedef std::map<CHotKeyItem, UInt32> HotKeyToIDMap;

	// true if screen is being used as a primary screen, false otherwise
	bool				m_isPrimary;
	int 				m_mouseScrollDelta;

	Display*			m_display;
	Window				m_root;
	Window				m_window;

	// true if mouse has entered the screen
	bool				m_isOnScreen;

	// screen shape stuff
	SInt32				m_x, m_y;
	SInt32				m_w, m_h;
	SInt32				m_xCenter, m_yCenter;

	// last mouse position
	SInt32				m_xCursor, m_yCursor;

	// keyboard stuff
	CXWindowsKeyState*	m_keyState;

	// hot key stuff
	HotKeyMap			m_hotKeys;
	HotKeyIDList		m_oldHotKeyIDs;
	HotKeyToIDMap		m_hotKeyToIDMap;

	// input focus stuff
	Window				m_lastFocus;
	int					m_lastFocusRevert;

	// input method stuff
	XIM					m_im;
	XIC					m_ic;
	KeyCode				m_lastKeycode;
	CFilteredKeycodes	m_filtered;

	// clipboards
	CXWindowsClipboard*	m_clipboard[kClipboardEnd];
	UInt32				m_sequenceNumber;

	// screen saver stuff
	CXWindowsScreenSaver*	m_screensaver;
	bool				m_screensaverNotify;

	// logical to physical button mapping.  m_buttons[i] gives the
	// physical button for logical button i+1.
	std::vector<unsigned char>	m_buttons;

	// true if global auto-repeat was enabled before we turned it off
	bool				m_autoRepeat;

	// stuff to workaround xtest being xinerama unaware.  attempting
	// to fake a mouse motion under xinerama may behave strangely,
	// especially if screen 0 is not at 0,0 or if faking a motion on
	// a screen other than screen 0.
	bool				m_xtestIsXineramaUnaware;
	bool				m_xinerama;

	// stuff to work around lost focus issues on certain systems
	// (ie: a MythTV front-end).
	bool				m_preserveFocus;

	// XKB extension stuff
	bool				m_xkb;
	int					m_xkbEventBase;

	// pointer to (singleton) screen.  this is only needed by
	// ioErrorHandler().
	static CXWindowsScreen*	s_screen;
};

#endif
