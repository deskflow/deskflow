#ifndef CXWINDOWSPRIMARYSCREEN_H
#define CXWINDOWSPRIMARYSCREEN_H

#include "KeyTypes.h"
#include "MouseTypes.h"
#include "CXWindowsScreen.h"
#include "IPrimaryScreen.h"

class CXWindowsPrimaryScreen : public CXWindowsScreen, public IPrimaryScreen {
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

  protected:
	// CXWindowsScreen overrides
	virtual void		onOpenDisplay();
	virtual void		onCloseDisplay();
	virtual void		eventThread(void*);

  private:
	void				selectEvents(Display*, Window) const;
	void				warpCursorNoLock(Display*,
								SInt32 xAbsolute, SInt32 yAbsolute);

	KeyModifierMask		mapModifier(unsigned int state) const;
	KeyID				mapKey(KeyCode, KeyModifierMask) const;
	ButtonID			mapButton(unsigned int button) const;

  private:
	CServer*			m_server;
	bool				m_active;
	Window				m_window;
};

#endif
