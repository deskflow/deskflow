#ifndef CMSWINDOWSSECONDARYSCREEN_H
#define CMSWINDOWSSECONDARYSCREEN_H

// ensure that we get SendInput()
#if _WIN32_WINNT <= 0x400
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x401
#endif

#include "CMSWindowsScreen.h"
#include "ISecondaryScreen.h"
#include "CMutex.h"
#include "CString.h"
#include "stdvector.h"

class IScreenReceiver;

class CMSWindowsSecondaryScreen : public CMSWindowsScreen,
							public ISecondaryScreen {
public:
	CMSWindowsSecondaryScreen(IScreenReceiver*);
	virtual ~CMSWindowsSecondaryScreen();

	// ISecondaryScreen overrides
	virtual void		run();
	virtual void		stop();
	virtual void		open();
	virtual void		close();
	virtual void		enter(SInt32 xAbsolute, SInt32 yAbsolute,
							KeyModifierMask mask);
	virtual void		leave();
	virtual void		keyDown(KeyID, KeyModifierMask);
	virtual void		keyRepeat(KeyID, KeyModifierMask, SInt32 count);
	virtual void		keyUp(KeyID, KeyModifierMask);
	virtual void		mouseDown(ButtonID);
	virtual void		mouseUp(ButtonID);
	virtual void		mouseMove(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual void		mouseWheel(SInt32 delta);
	virtual void		setClipboard(ClipboardID, const IClipboard*);
	virtual void		grabClipboard(ClipboardID);
	virtual void		screenSaver(bool activate);
	virtual void		getMousePos(SInt32& x, SInt32& y) const;
	virtual void		getShape(SInt32&, SInt32&, SInt32&, SInt32&) const;
	virtual SInt32		getJumpZoneSize() const;
	virtual void		getClipboard(ClipboardID, IClipboard*) const;

protected:
	// CMSWindowsScreen overrides
	virtual bool		onPreDispatch(const CEvent* event);
	virtual bool		onEvent(CEvent* event);
	virtual CString		getCurrentDesktopName() const;

private:
	enum EKeyAction { kPress, kRelease, kRepeat };
	class Keystroke {
	public:
		UINT			m_virtualKey;
		bool			m_press;
		bool			m_repeat;
	};
	typedef std::vector<Keystroke> Keystrokes;

	void				showWindow();
	void				hideWindow();

	// warp the mouse to the specified position
	void				warpCursor(SInt32 x, SInt32 y);

	// check clipboard ownership and, if necessary, tell the receiver
	// of a grab.
	void				checkClipboard();

	// create/destroy window
	// also attach to desktop;  this destroys and recreates the window
	// as necessary.
	void				createWindow();
	void				destroyWindow();

	// start/stop watch for screen saver changes
	void				installScreenSaver();
	void				uninstallScreenSaver();

	// open/close desktop (for windows 95/98/me)
	bool				openDesktop();
	void				closeDesktop();

	// make desk the thread desktop (for windows NT/2000/XP)
	bool				switchDesktop(HDESK desk);

	// get calling thread to use the input desktop
	void				syncDesktop() const;

	// returns true iff there appear to be multiple monitors
	bool				isMultimon() const;

	// key and button queries and operations
	DWORD				mapButton(ButtonID button, bool press) const;
	KeyModifierMask		mapKey(Keystrokes&, UINT& virtualKey, KeyID,
							KeyModifierMask, EKeyAction) const;
	void				doKeystrokes(const Keystrokes&, SInt32 count);

	void				releaseKeys();
	void				updateKeys();
	void				updateModifiers();
	void				toggleKey(UINT virtualKey, KeyModifierMask mask);
	UINT				virtualKeyToScanCode(UINT& virtualKey);
	bool				isExtendedKey(UINT virtualKey);
	void				sendKeyEvent(UINT virtualKey, bool press);

private:
	CMutex				m_mutex;
	IScreenReceiver*	m_receiver;

	// true if windows 95/98/me
	bool				m_is95Family;

	// true if system appears to have multiple monitors
	bool				m_multimon;

	// the main loop's thread id
	DWORD				m_threadID;

	// the timer used to check for desktop switching
	UINT				m_timer;

	// the thread id of the last attached thread
	mutable DWORD		m_lastThreadID;

	// the current desk and it's name
	HDESK				m_desk;
	CString				m_deskName;

	// our window (for getting clipboard changes)
	HWND				m_window;

	// m_active is true if this screen has been entered
	bool				m_active;

	// clipboard stuff
	HWND				m_nextClipboardWindow;
	HWND				m_clipboardOwner;

	// virtual key states
	BYTE				m_keys[256];

	// current active modifiers
	KeyModifierMask		m_mask;
};

#endif
