#ifndef CXWINDOWSPRIMARYSCREEN_H
#define CXWINDOWSPRIMARYSCREEN_H

#include "CXWindowsScreen.h"
#include "IPrimaryScreen.h"
#include "MouseTypes.h"

class IScreenReceiver;
class IPrimaryScreenReceiver;

class CXWindowsPrimaryScreen : public CXWindowsScreen, public IPrimaryScreen {
public:
	CXWindowsPrimaryScreen(IScreenReceiver*, IPrimaryScreenReceiver*);
	virtual ~CXWindowsPrimaryScreen();

	// IPrimaryScreen overrides
	virtual void		run();
	virtual void		stop();
	virtual void		open();
	virtual void		close();
	virtual void		enter(SInt32 xAbsolute, SInt32 yAbsolute, bool);
	virtual bool		leave();
	virtual void		reconfigure(UInt32 activeSides);
	virtual void		warpCursor(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual void		setClipboard(ClipboardID, const IClipboard*);
	virtual void		grabClipboard(ClipboardID);
	virtual void		getClipboard(ClipboardID, IClipboard*) const;
	virtual KeyModifierMask	getToggleMask() const;
	virtual bool		isLockedToScreen() const;

protected:
	// CXWindowsScreen overrides
	virtual bool		onPreDispatch(const CEvent* event);
	virtual bool		onEvent(CEvent* event);
	virtual void		onUnexpectedClose();
	virtual void		onLostClipboard(ClipboardID);

private:
	SInt32				getJumpZoneSize() const;

	void				warpCursorToCenter();
	void				warpCursorNoFlush(Display*,
							SInt32 xAbsolute, SInt32 yAbsolute);

	void				enterNoWarp();
	bool				showWindow();
	void				hideWindow();

	// check clipboard ownership and, if necessary, tell the receiver
	// of a grab.
	void				checkClipboard();

	// create/destroy window
	void				createWindow();
	void				destroyWindow();

	// start/stop watch for screen saver changes
	void				installScreenSaver();
	void				uninstallScreenSaver();

	void				selectEvents(Display*, Window) const;
	void				doSelectEvents(Display*, Window) const;

	KeyModifierMask		mapModifier(unsigned int state) const;
	KeyID				mapKey(XKeyEvent*) const;
	ButtonID			mapButton(unsigned int button) const;

	void				updateModifierMap(Display* display);

	class CKeyEventInfo {
	public:
		int				m_event;
		Window			m_window;
		Time			m_time;
		KeyCode			m_keycode;
	};
	static Bool			findKeyEvent(Display*, XEvent* xevent, XPointer arg);

private:
	IScreenReceiver*		m_receiver;
	IPrimaryScreenReceiver*	m_primaryReceiver;

	bool				m_active;
	Window				m_window;

	// atom for screen saver messages
	Atom				m_atomScreenSaver;

	// note toggle keys that toggle on up/down (false) or on
	// transition (true)
	bool				m_numLockHalfDuplex;
	bool				m_capsLockHalfDuplex;

	// masks that indicate which modifier bits are for toggle keys
	unsigned int		m_numLockMask;
	unsigned int		m_capsLockMask;
	unsigned int		m_scrollLockMask;

	// last mouse position
	SInt32				m_x, m_y;

	// position of center pixel of screen
	SInt32				m_xCenter, m_yCenter;
};

#endif
