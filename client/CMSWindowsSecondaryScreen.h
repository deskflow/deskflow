#ifndef CMSWINDOWSSECONDARYSCREEN_H
#define CMSWINDOWSSECONDARYSCREEN_H

#include "CMSWindowsScreen.h"
#include "ISecondaryScreen.h"
#include <map>
#include <vector>

class CMSWindowsSecondaryScreen : public CMSWindowsScreen, public ISecondaryScreen {
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
	virtual void		getSize(SInt32* width, SInt32* height) const;
	virtual SInt32		getJumpZoneSize() const;
	virtual void		getClipboard(ClipboardID, IClipboard*) const;

protected:
	// CMSWindowsScreen overrides
	virtual bool		onPreTranslate(MSG*);
	virtual LRESULT		onEvent(HWND, UINT, WPARAM, LPARAM);
	virtual void		onOpenDisplay();
	virtual void		onCloseDisplay();

private:
	enum EKeyAction { kPress, kRelease, kRepeat };
	class Keystroke {
	public:
		UINT			m_virtualKey;
		bool			m_press;
		bool			m_repeat;
	};
	typedef std::vector<Keystroke> Keystrokes;
	
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
	CClient*			m_client;
	HWND				m_window;
	HWND				m_nextClipboardWindow;
	HWND				m_clipboardOwner;

	// thread id of the event loop thread
	DWORD				m_threadID;

	// virtual key states
	BYTE				m_keys[256];

	// current active modifiers
	KeyModifierMask		m_mask;
};

#endif
