#ifndef CXWINDOWSPRIMARYSCREEN_H
#define CXWINDOWSPRIMARYSCREEN_H

#include "KeyTypes.h"
#include "MouseTypes.h"
#include "IPrimaryScreen.h"
#include <X11/Xlib.h>

class CThread;

class CXWindowsPrimaryScreen : public IPrimaryScreen {
  public:
	CXWindowsPrimaryScreen();
	virtual ~CXWindowsPrimaryScreen();

	// IPrimaryScreen overrides
	virtual void		open(CServer*);
	virtual void		close();
	virtual void		enter(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual void		leave();
	virtual void		warpCursor(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual void		getSize(SInt32* width, SInt32* height) const;
	virtual SInt32		getJumpZoneSize() const;

  private:
	void				selectEvents(Window) const;
	Cursor				createBlankCursor();

	void				eventThread(void*);
	KeyModifierMask		mapModifier(unsigned int state) const;
	KeyID				mapKey(KeyCode, KeyModifierMask) const;
	ButtonID			mapButton(unsigned int button) const;

  private:
	CServer*			m_server;
	CThread*			m_eventThread;
	Display*			m_display;
	int					m_screen;
	Window				m_root;
	SInt32				m_w, m_h;
	Window				m_window;
	bool				m_active;
};

#endif
