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
	return CUnicode::UTF8ToText(data);
}

CString
CMSWindowsClipboardTextConverter::doToIClipboard(const CString& data) const
{
	return CUnicode::textToUTF8(data);
}
