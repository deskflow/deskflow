#ifndef CMSWINDOWSCLIPBOARDTEXTCONVERTER_H
#define CMSWINDOWSCLIPBOARDTEXTCONVERTER_H

#include "CMSWindowsClipboardAnyTextConverter.h"

class CMSWindowsClipboardTextConverter :
				public CMSWindowsClipboardAnyTextConverter {
public:
	CMSWindowsClipboardTextConverter();
	virtual ~CMSWindowsClipboardTextConverter();

	// IMSWindowsClipboardConverter overrides
	virtual UINT		getWin32Format() const;

protected:
	// CMSWindowsClipboardAnyTextConverter overrides
	virtual CString		doFromIClipboard(const CString&) const;
	virtual CString		doToIClipboard(const CString&) const;
};

#endif
