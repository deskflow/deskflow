#ifndef CMSWINDOWSSECONDARYSCREEN_H
#define CMSWINDOWSSECONDARYSCREEN_H

#include "CMSWindowsScreen.h"
#include "ISecondaryScreen.h"

class CMSWindowsSecondaryScreen : public CMSWindowsScreen, public ISecondaryScreen {
  public:
	CMSWindowsSecondaryScreen();
	virtual ~CMSWindowsSecondaryScreen();

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
	// CMSWindowsScreen overrides
	virtual bool		onEvent(MSG*);
	virtual void		onOpenDisplay();
	virtual void		onCloseDisplay();

  private:
	UINT				mapKey(KeyID, KeyModifierMask) const;

  private:
	CClient*			m_client;
	HWND				m_window;
};

#endif
