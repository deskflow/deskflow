#ifndef CMSWINDOWSCLIPBOARD_H
#define CMSWINDOWSCLIPBOARD_H

#include "IClipboard.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class CMSWindowsClipboard : public IClipboard {
public:
	CMSWindowsClipboard(HWND window);
	virtual ~CMSWindowsClipboard();

	// IClipboard overrides
	virtual bool		empty();
	virtual void		add(EFormat, const CString& data);
	virtual bool		open(Time) const;
	virtual void		close() const;
	virtual Time		getTime() const;
	virtual bool		has(EFormat) const;
	virtual CString		get(EFormat) const;

private:
	UINT				convertFormatToWin32(EFormat) const;
	HANDLE				convertTextToWin32(const CString& data) const;
	CString				convertTextFromWin32(HANDLE) const;

private:
	HWND				m_window;
	mutable Time		m_time;
};

#endif
