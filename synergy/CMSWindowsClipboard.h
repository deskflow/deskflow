#ifndef CMSWINDOWSCLIPBOARD_H
#define CMSWINDOWSCLIPBOARD_H

#include "IClipboard.h"

class CMSWindowsClipboard : public IClipboard {
  public:
	CMSWindowsClipboard();
	virtual ~CMSWindowsClipboard();

	// IClipboard overrides
	virtual void		open();
	virtual void		close();
	virtual void		add(EFormat, const CString& data);
	virtual bool		has(EFormat) const;
	virtual CString		get(EFormat) const;
};

#endif
