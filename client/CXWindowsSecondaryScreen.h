#ifndef CXWINDOWSSECONDARYSCREEN_H
#define CXWINDOWSSECONDARYSCREEN_H

#include "CXWindowsScreen.h"
#include "ISecondaryScreen.h"

class CXWindowsSecondaryScreen : public CXWindowsScreen, public ISecondaryScreen {
  public:
	CXWindowsSecondaryScreen();
	virtual ~CXWindowsSecondaryScreen();

	// ISecondaryScreen overrides
	virtual void		run();
	virtual void		stop();
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

  protected:
	// CXWindowsScreen overrides
	virtual void		onOpenDisplay();
	virtual void		onCloseDisplay();

  private:
	void				leaveNoLock(Display*);
	KeyCode				mapKey(KeyID, KeyModifierMask) const;
	unsigned int		mapButton(ButtonID button) const;

  private:
	CClient*			m_client;
	Window				m_window;
};

#endif
