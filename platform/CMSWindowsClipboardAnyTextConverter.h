#ifndef CMSWINDOWSCLIPBOARDANYTEXTCONVERTER_H
#define CMSWINDOWSCLIPBOARDANYTEXTCONVERTER_H

#include "CMSWindowsClipboard.h"

//! Convert to/from some text encoding
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
	//! Convert from IClipboard format
	/*!
	Do UTF-8 conversion only.  Memory handle allocation and
	linefeed conversion is done by this class.  doFromIClipboard()
	must include the nul terminator in the returned string (not
	including the CString's nul terminator).
	*/
	virtual CString		doFromIClipboard(const CString&) const = 0;

	//! Convert to IClipboard format
	/*!
	Do UTF-8 conversion only.  Memory handle allocation and
	linefeed conversion is done by this class.
	*/
	virtual CString		doToIClipboard(const CString&) const = 0;

private:
	CString				convertLinefeedToWin32(const CString&) const;
	CString				convertLinefeedToUnix(const CString&) const;
};

#endif
