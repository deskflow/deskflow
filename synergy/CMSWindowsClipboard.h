#ifndef CMSWINDOWSCLIPBOARD_H
#define CMSWINDOWSCLIPBOARD_H

#include "IClipboard.h"
#include <windows.h>

class CMSWindowsClipboard : public IClipboard {
public:
	CMSWindowsClipboard(HWND window);
	virtual ~CMSWindowsClipboard();

	// IClipboard overrides
	virtual bool		open();
	virtual void		close();
	virtual void		add(EFormat, const CString& data);
	virtual bool		has(EFormat) const;
	virtual CString		get(EFormat) const;

private:
	UINT				convertFormatToWin32(EFormat) const;
	HANDLE				convertTextToWin32(const CString& data) const;
	CString				convertTextFromWin32(HANDLE) const;

private:
	HWND				m_window;
};

#endif
