#ifndef CXWINDOWSCLIPBOARD_H
#define CXWINDOWSCLIPBOARD_H

#include "IClipboard.h"

class CXWindowsClipboard : public IClipboard {
  public:
	CXWindowsClipboard();
	virtual ~CXWindowsClipboard();

	// IClipboard overrides
	virtual bool		open();
	virtual void		close();
	virtual void		add(EFormat, const CString& data);
	virtual bool		has(EFormat) const;
	virtual CString		get(EFormat) const;
};

#endif
