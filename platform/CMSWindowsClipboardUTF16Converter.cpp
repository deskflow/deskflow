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
	// convert and add nul terminator
	return CUnicode::UTF8ToUTF16(data).append(sizeof(wchar_t), 0);
}

CString
CMSWindowsClipboardUTF16Converter::doToIClipboard(const CString& data) const
{
	// convert and strip nul terminator
	CString dst = CUnicode::UTF16ToUTF8(data);
	if (dst.size() > 0 && dst[size() - 1] == '\0') {
		dst.erase(dst.size() - 1);
	}
	return dst;
}
