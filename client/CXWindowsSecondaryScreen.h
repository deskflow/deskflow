#ifndef CXWINDOWSSECONDARYSCREEN_H
#define CXWINDOWSSECONDARYSCREEN_H

#include "CXWindowsScreen.h"
#include "ISecondaryScreen.h"
#include "stdmap.h"
#include "stdvector.h"

class CXWindowsSecondaryScreen : public CXWindowsScreen,
							public ISecondaryScreen {
public:
	CXWindowsSecondaryScreen();
	virtual ~CXWindowsSecondaryScreen();

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
	virtual void		getMousePos(SInt32* x, SInt32* y) const;
	virtual void		getSize(SInt32* width, SInt32* height) const;
	virtual SInt32		getJumpZoneSize() const;
	virtual void		getClipboard(ClipboardID, IClipboard*) const;

protected:
	// CXWindowsScreen overrides
	virtual void		onOpenDisplay(Display*);
	virtual CXWindowsClipboard*
						createClipboard(ClipboardID);
	virtual void		onCloseDisplay(Display*);
	virtual void		onLostClipboard(ClipboardID);

private:
	enum EKeyAction { kPress, kRelease, kRepeat };
	class KeyCodeMask {
	public:
		KeyCode			m_keycode;
		unsigned int	m_keyMask;
		unsigned int	m_keyMaskMask;
	};
	class Keystroke {
	public:
		KeyCode			m_keycode;
		Bool			m_press;
		bool			m_repeat;
	};
	typedef std::vector<Keystroke> Keystrokes;
	typedef std::vector<KeyCode> KeyCodes;
	typedef std::map<KeyID, KeyCodeMask> KeyCodeMap;
	typedef std::map<KeyCode, unsigned int> ModifierMap;

	void				leaveNoLock(Display*);
	unsigned int		mapButton(ButtonID button) const;

	unsigned int		mapKey(Keystrokes&, KeyCode&, KeyID,
							KeyModifierMask, EKeyAction) const;
	bool				findKeyCode(KeyCode&, unsigned int&,
							KeyID id, unsigned int) const;
	void				doKeystrokes(const Keystrokes&, SInt32 count);
	unsigned int		maskToX(KeyModifierMask) const;

	void				updateKeys(Display* display);
	void				updateKeycodeMap(Display* display);
	void				updateModifiers(Display* display);
	void				updateModifierMap(Display* display);
	void				toggleKey(Display*, KeySym, unsigned int mask);
	static bool			isToggleKeysym(KeySym);

private:
	CClient*			m_client;
	Window				m_window;

	// note toggle keys that toggles on up/down (false) or on
	// transition (true)
	bool				m_numLockHalfDuplex;
	bool				m_capsLockHalfDuplex;

	// set entries indicate keys that are pressed.  indexed by keycode.
	bool				m_keys[256];

	// current active modifiers (X key masks)
	unsigned int		m_mask;

	// maps key IDs to X keycodes and the X modifier key mask needed
	// to generate the right keysym
	KeyCodeMap			m_keycodeMap;

	// the modifiers that have keys bound to them
	unsigned int		m_modifierMask;

	// set bits indicate modifiers that toggle (e.g. caps-lock)
	unsigned int		m_toggleModifierMask;

	// masks that indicate which modifier bits are for toggle keys
	unsigned int		m_numLockMask;
	unsigned int		m_capsLockMask;
	unsigned int		m_scrollLockMask;

	// map X modifier key indices to the key codes bound to them
	unsigned int		m_keysPerModifier;
	KeyCodes			m_modifierToKeycode;

	// maps keycodes to modifier indices
	ModifierMap			m_keycodeToModifier;
};

#endif
