#ifndef CXWINDOWSCLIPBOARDTEXTCONVERTER_H
#define CXWINDOWSCLIPBOARDTEXTCONVERTER_H

#include "CXWindowsClipboard.h"

class CXWindowsClipboardTextConverter : public IXWindowsClipboardConverter {
public:
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
