#include "CMSWindowsClipboardUTF16Converter.h"
#include "CUnicode.h"

//
// CMSWindowsClipboardUTF16Converter
//

CMSWindowsClipboardUTF16Converter::CMSWindowsClipboardUTF16Converter()
{
	// do nothing
}

CMSWindowsClipboardUTF16Converter::~CMSWindowsClipboardUTF16Converter()
{
	// do nothing
}

UINT
CMSWindowsClipboardUTF16Converter::getWin32Format() const
{
	return CF_UNICODETEXT;
}

CString
CMSWindowsClipboardUTF16Converter::doFromIClipboard(const CString& data) const
{
	return CUnicode::UTF8ToUTF16(data);
}

CString
CMSWindowsClipboardUTF16Converter::doToIClipboard(const CString& data) const
{
	return CUnicode::UTF16ToUTF8(data);
}
