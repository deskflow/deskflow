#ifndef CXWINDOWSCLIPBOARDUTF8CONVERTER_H
#define CXWINDOWSCLIPBOARDUTF8CONVERTER_H

#include "CXWindowsClipboard.h"

class CXWindowsClipboardUTF8Converter : public IXWindowsClipboardConverter {
public:
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
