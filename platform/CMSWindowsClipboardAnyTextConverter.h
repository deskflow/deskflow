#ifndef CMSWINDOWSCLIPBOARDANYTEXTCONVERTER_H
#define CMSWINDOWSCLIPBOARDANYTEXTCONVERTER_H

#include "CMSWindowsClipboard.h"

class CMSWindowsClipboardAnyTextConverter :
				public IMSWindowsClipboardConverter {
public:
	CMSWindowsClipboardAnyTextConverter();
	virtual ~CMSWindowsClipboardAnyTextConverter();

	// IMSWindowsClipboardConverter overrides
	virtual IClipboard::EFormat
						getFormat() const;
	virtual UINT		getWin32Format() const = 0;
	virtual HANDLE		fromIClipboard(const CString&) const;
	virtual CString		toIClipboard(HANDLE) const;

protected:
	// do UTF-8 conversion only.  memory handle allocation and
	// linefeed conversion is done by this class.  doFromIClipboard()
	// must include the nul terminator in the returned string (not
	// including the CString's nul terminator).
	virtual CString		doFromIClipboard(const CString&) const = 0;
	virtual CString		doToIClipboard(const CString&) const = 0;

private:
	CString				convertLinefeedToWin32(const CString&) const;
	CString				convertLinefeedToUnix(const CString&) const;
};

#endif
