#ifndef CSCREENPROXY_H
#define CSCREENPROXY_H

#include "IScreen.h"

class ISocket;

class CScreenProxy : public IScreen {
  public:
	CScreenProxy(const CString& name, ISocket*);
	virtual ~CScreenProxy();

	// IScreen overrides
	virtual void		open(bool);
	virtual void		close();
	virtual void		enterScreen(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual void		leaveScreen();
	virtual void		warpCursor(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual void		setClipboard(const IClipboard*);
	virtual void		onScreenSaver(bool show);
	virtual void		onKeyDown(KeyID, KeyModifierMask);
	virtual void		onKeyRepeat(KeyID, KeyModifierMask, SInt32 count);
	virtual void		onKeyUp(KeyID, KeyModifierMask);
	virtual void		onMouseDown(ButtonID);
	virtual void		onMouseUp(ButtonID);
	virtual void		onMouseMove(SInt32 xAbsolute, SInt32 yAbsolute);
	virtual void		onMouseWheel(SInt32 delta);
	virtual void		onClipboardChanged();
	virtual CString		getName() const;
	virtual void		getSize(SInt32* width, SInt32* height) const;
	virtual void		getClipboard(IClipboard*) const;

  private:
	void				onRead();

  private:
	CString				m_name;
	ISocket*			m_socket;
	SInt32				m_w, m_h;
};

#endif
