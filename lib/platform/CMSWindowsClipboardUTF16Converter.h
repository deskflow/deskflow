#ifndef CMSWINDOWSCLIPBOARDUTF16CONVERTER_H
#define CMSWINDOWSCLIPBOARDUTF16CONVERTER_H

#include "CMSWindowsClipboardAnyTextConverter.h"

//! Convert to/from UTF-16 encoding
class CMSWindowsClipboardUTF16Converter :
				public CMSWindowsClipboardAnyTextConverter {
public:
	CMSWindowsClipboardUTF16Converter();
	virtual ~CMSWindowsClipboardUTF16Converter();

	// IMSWindowsClipboardConverter overrides
	virtual UINT		getWin32Format() const;

protected:
	// CMSWindowsClipboardAnyTextConverter overrides
	virtual CString		doFromIClipboard(const CString&) const;
	virtual CString		doToIClipboard(const CString&) const;
};

#endif
