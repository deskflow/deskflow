#include "CXWindowsClipboardUCS2Converter.h"
#include "CUnicode.h"

//
// CXWindowsClipboardUCS2Converter
//

CXWindowsClipboardUCS2Converter::CXWindowsClipboardUCS2Converter(
				Display* display, const char* name) :
	m_atom(XInternAtom(display, name, False))
{
	// do nothing
}

CXWindowsClipboardUCS2Converter::~CXWindowsClipboardUCS2Converter()
{
	// do nothing
}

IClipboard::EFormat
CXWindowsClipboardUCS2Converter::getFormat() const
{
	return IClipboard::kText;
}

Atom
CXWindowsClipboardUCS2Converter::getAtom() const
{
	return m_atom;
}

int
CXWindowsClipboardUCS2Converter::getDataSize() const
{
	return 16;
}

CString
CXWindowsClipboardUCS2Converter::fromIClipboard(const CString& data) const
{
	return CUnicode::UTF8ToUCS2(data);
}

CString
CXWindowsClipboardUCS2Converter::toIClipboard(const CString& data) const
{
	return CUnicode::UCS2ToUTF8(data);
}
