#ifndef CXWINDOWSCLIPBOARDUTF8CONVERTER_H
#define CXWINDOWSCLIPBOARDUTF8CONVERTER_H

#include "CXWindowsClipboard.h"

//! Convert to/from UTF-8 encoding
class CXWindowsClipboardUTF8Converter : public IXWindowsClipboardConverter {
public:
	/*!
	\c name is converted to an atom and that is reported by getAtom().
	*/
	CXWindowsClipboardUTF8Converter(Display* display, const char* name);
	virtual ~CXWindowsClipboardUTF8Converter();

	// IXWindowsClipboardConverter overrides
	virtual IClipboard::EFormat
						getFormat() const;
	virtual Atom		getAtom() const;
	virtual int			getDataSize() const;
	virtual CString		fromIClipboard(const CString&) const;
	virtual CString		toIClipboard(const CString&) const;

private:
	Atom				m_atom;
};

#endif
