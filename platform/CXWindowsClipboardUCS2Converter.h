#ifndef CXWINDOWSCLIPBOARDUCS2CONVERTER_H
#define CXWINDOWSCLIPBOARDUCS2CONVERTER_H

#include "CXWindowsClipboard.h"

//! Convert to/from UCS-2 encoding
class CXWindowsClipboardUCS2Converter : public IXWindowsClipboardConverter {
public:
	/*!
	\c name is converted to an atom and that is reported by getAtom().
	*/
	CXWindowsClipboardUCS2Converter(Display* display, const char* name);
	virtual ~CXWindowsClipboardUCS2Converter();

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
