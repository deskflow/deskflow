#ifndef CUNIXXSCREEN_H
#define CUNIXXSCREEN_H

#include "CXScreen.h"

class CUnixXScreen : public CXScreen {
  public:
	CUnixXScreen(const CString& name);
	virtual ~CUnixXScreen();

  protected:
	virtual void		onOpen(bool isPrimary);
	virtual void		onClose();
};

#endif
