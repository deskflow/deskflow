#ifndef CMSWINDOWSSECONDARYSCREEN_H
#define CMSWINDOWSSECONDARYSCREEN_H

#include "CMSWindowsScreen.h"
#include "ISecondaryScreen.h"
#include "CMutex.h"
#include "CString.h"
#include "stdvector.h"

class CMSWindowsSecondaryScreen : public CMSWindowsScreen,
							public ISecondaryScreen {
public:
	CMSWindowsSecondaryScreen();
	virtual ~CMSWindowsSecondaryScreen();

	// ISecondaryScreen overrides
	virtual void		run();
	virtual void		stop();
	virtual void		open(CClient*);
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
	virtual void		getMousePos(SInt32& x, SInt32& y) const;
	virtual void		getShape(SInt32&, SInt32&, SInt32&, SInt32&) const;
	virtual SInt32		getJumpZoneSize() const;
	virtual void		getClipboard(ClipboardID, IClipboard*) const;

protected:
	// CMSWindowsScreen overrides
	virtual bool		onPreTranslate(MSG*);
	virtual LRESULT		onEvent(HWND, UINT, WPARAM, LPARAM);
	virtual void		onOpenDisplay();
	virtual void		onCloseDisplay();
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

	void				onEnter(SInt32 x, SInt32 y);
	void				onLeave();

	// open/close desktop (for windows 95/98/me)
	bool				openDesktop();
	void				closeDesktop();

	// make desk the thread desktop (for windows NT/2000/XP)
	bool				switchDesktop(HDESK desk);

	// get calling thread to use the input desktop
	void				syncDesktop() const;

	// key and button queries and operations
	DWORD				mapButton(ButtonID button, bool press) const;
	KeyModifierMask		mapKey(Keystrokes&, UINT& virtualKey, KeyID,
							KeyModifierMask, EKeyAction) const;
	void				doKeystrokes(const Keystrokes&, SInt32 count);

	void				updateKeys();
	void				updateModifiers();
	void				toggleKey(UINT virtualKey, KeyModifierMask mask);
	UINT				virtualKeyToScanCode(UINT& virtualKey);
	bool				isExtendedKey(UINT virtualKey);
	void				sendKeyEvent(UINT virtualKey, bool press);

private:
	CMutex				m_mutex;
	CClient*			m_client;

	// true if windows 95/98/me
	bool				m_is95Family;

	// the main loop's thread id
	DWORD				m_threadID;

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
