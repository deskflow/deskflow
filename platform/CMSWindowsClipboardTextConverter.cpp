#include "CMSWindowsClipboardTextConverter.h"
#include "CUnicode.h"

//
// CMSWindowsClipboardTextConverter
//

CMSWindowsClipboardTextConverter::CMSWindowsClipboardTextConverter()
{
	// do nothing
}

CMSWindowsClipboardTextConverter::~CMSWindowsClipboardTextConverter()
{
	// do nothing
}

UINT
CMSWindowsClipboardTextConverter::getWin32Format() const
{
	return CF_TEXT;
}

CString
CMSWindowsClipboardTextConverter::doFromIClipboard(const CString& data) const
{
	// convert and add nul terminator
	return CUnicode::UTF8ToText(data) += '\0';
}

CString
CMSWindowsClipboardTextConverter::doToIClipboard(const CString& data) const
{
	// convert and strip nul terminator
	CString dst = CUnicode::textToUTF8(data);
	if (dst.size() > 0 && dst[size() - 1] == '\0') {
		dst.erase(dst.size() - 1);
	}
	return dst;
}
