#ifndef CXWINDOWSCLIPBOARDTEXTCONVERTER_H
#define CXWINDOWSCLIPBOARDTEXTCONVERTER_H

#include "CXWindowsClipboard.h"

//! Convert to/from locale text encoding
class CXWindowsClipboardTextConverter : public IXWindowsClipboardConverter {
public:
	/*!
	\c name is converted to an atom and that is reported by getAtom().
	*/
	CXWindowsClipboardTextConverter(Display* display, const char* name);
	virtual ~CXWindowsClipboardTextConverter();

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
