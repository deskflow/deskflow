#include "CXWindowsClipboardTextConverter.h"
#include "CUnicode.h"

//
// CXWindowsClipboardTextConverter
//

CXWindowsClipboardTextConverter::CXWindowsClipboardTextConverter(
				Display* display, const char* name) :
	m_atom(XInternAtom(display, name, False))
{
	// do nothing
}

CXWindowsClipboardTextConverter::~CXWindowsClipboardTextConverter()
{
	// do nothing
}

IClipboard::EFormat
CXWindowsClipboardTextConverter::getFormat() const
{
	return IClipboard::kText;
}

Atom
CXWindowsClipboardTextConverter::getAtom() const
{
	return m_atom;
}

int
CXWindowsClipboardTextConverter::getDataSize() const
{
	return 8;
}

CString
CXWindowsClipboardTextConverter::fromIClipboard(const CString& data) const
{
	return CUnicode::UTF8ToText(data);
}

CString
CXWindowsClipboardTextConverter::toIClipboard(const CString& data) const
{
	return CUnicode::textToUTF8(data);
}
