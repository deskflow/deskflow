#ifndef CXWINDOWSSECONDARYSCREEN_H
#define CXWINDOWSSECONDARYSCREEN_H

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
	virtual void		warpCursor(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual void		onKeyDown(KeyID, KeyModifierMask);
	virtual void		onKeyRepeat(KeyID, KeyModifierMask, SInt32 count);
	virtual void		onKeyUp(KeyID, KeyModifierMask);
	virtual void		onMouseDown(ButtonID);
	virtual void		onMouseUp(ButtonID);
	virtual void		onMouseMove(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual void		onMouseWheel(SInt32 delta);
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
};

#endif
