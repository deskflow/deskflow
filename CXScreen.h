#ifndef CXSCREEN_H
#define CXSCREEN_H

#include "IScreen.h"
#include <X11/Xlib.h>

class CXScreen : public IScreen {
  public:
	CXScreen(const CString& name);
	virtual ~CXScreen();

	// IScreen overrides
	virtual void		open(bool isPrimary);
	virtual void		close();
	virtual void		enterScreen(SInt32 x, SInt32 y);
	virtual void		leaveScreen();
	virtual void		warpCursor(SInt32 x, SInt32 y);
	virtual void		setClipboard(const IClipboard*);
	virtual void		onScreenSaver(bool);
	virtual void		onKeyDown(KeyID);
	virtual void		onKeyRepeat(KeyID, SInt32);
	virtual void		onKeyUp(KeyID);
	virtual void		onKeyToggle(KeyToggleMask);
	virtual void		onMouseDown(ButtonID);
	virtual void		onMouseUp(ButtonID);
	virtual void		onMouseMove(SInt32, SInt32);
	virtual void		onMouseWheel(SInt32);
	virtual void		onClipboardChanged();
	virtual CString		getName() const;
	virtual void		getSize(SInt32* width, SInt32* height) const;
	virtual void		getClipboard(IClipboard*) const;

  protected:
	// primary screen implementations
	virtual void		openPrimary();
	virtual void		closePrimary();
	virtual void		enterScreenPrimary(SInt32 x, SInt32 y);
	virtual void		leaveScreenPrimary();
	virtual void		setClipboardPrimary(const IClipboard*);
	virtual void		onScreenSaverPrimary(bool);

	// secondary screen implementations
	virtual void		openSecondary();
	virtual void		closeSecondary();
	virtual void		enterScreenSecondary(SInt32 x, SInt32 y);
	virtual void		leaveScreenSecondary();
	virtual void		setClipboardSecondary(const IClipboard*);
	virtual void		onScreenSaverSecondary(bool);

	// get the display
	Display*			getDisplay() const;

	// process X events from the display
	void				onEvents();

	// called by open() and close().  override to hook up and unhook the
	// display connection to the event queue.  call onEvents() when events
	// are available.
	virtual void		onOpen(bool isPrimary) = 0;
	virtual void		onClose() = 0;

  private:
	void				selectEvents(Window) const;
	KeyID				mapKey(unsigned int keycode) const;
	ButtonID			mapButton(unsigned int button) const;
	void				onPrimaryEvents();
	void				onSecondaryEvents();

  private:
	CString				m_name;
	Display*			m_display;
	int					m_screen;
	bool				m_primary;
	SInt32				m_w, m_h;

	// stuff for primary screens
	Window				m_window;
	bool				m_active;
};

#endif
