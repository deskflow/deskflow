#include "CXWindowsClipboardUTF8Converter.h"

//
// CXWindowsClipboardUTF8Converter
//

CXWindowsClipboardUTF8Converter::CXWindowsClipboardUTF8Converter(
				Display* display, const char* name) :
	m_atom(XInternAtom(display, name, False))
{
	// do nothing
}

CXWindowsClipboardUTF8Converter::~CXWindowsClipboardUTF8Converter()
{
	// do nothing
}

IClipboard::EFormat
CXWindowsClipboardUTF8Converter::getFormat() const
{
	return IClipboard::kText;
}

Atom
CXWindowsClipboardUTF8Converter::getAtom() const
{
	return m_atom;
}

int
CXWindowsClipboardUTF8Converter::getDataSize() const
{
	return 8;
}

CString
CXWindowsClipboardUTF8Converter::fromIClipboard(const CString& data) const
{
	return data;
}

CString
CXWindowsClipboardUTF8Converter::toIClipboard(const CString& data) const
{
	return data;
}
