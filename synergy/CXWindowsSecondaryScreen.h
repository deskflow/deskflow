#ifndef CXWINDOWSSECONDARYSCREEN_H
#define CXWINDOWSSECONDARYSCREEN_H

#include "CMutex.h"
#include "ISecondaryScreen.h"
#include <X11/Xlib.h>

class CThread;

class CXWindowsSecondaryScreen : public ISecondaryScreen {
  public:
	CXWindowsSecondaryScreen();
	virtual ~CXWindowsSecondaryScreen();

	// ISecondaryScreen overrides
	virtual void		open(CClient*);
	virtual void		close();
	virtual void		enter(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual void		leave();
	virtual void		keyDown(KeyID, KeyModifierMask);
	virtual void		keyRepeat(KeyID, KeyModifierMask, SInt32 count);
	virtual void		keyUp(KeyID, KeyModifierMask);
	virtual void		mouseDown(ButtonID);
	virtual void		mouseUp(ButtonID);
	virtual void		mouseMove(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual void		mouseWheel(SInt32 delta);
	virtual void		getSize(SInt32* width, SInt32* height) const;
	virtual SInt32		getJumpZoneSize() const;

  private:
	Cursor				createBlankCursor();
	void				eventThread(void*);
	KeyCode				mapKey(KeyID, KeyModifierMask) const;
	unsigned int		mapButton(ButtonID button) const;

  private:
	CClient*			m_client;
	CThread*			m_eventThread;
	Display*			m_display;
	int					m_screen;
	Window				m_root;
	Window				m_window;
	SInt32				m_w, m_h;

	// X is not thread safe
	CMutex				m_mutex;
};

#endif
