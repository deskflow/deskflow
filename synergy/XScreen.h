#ifndef XSCREEN_H
#define XSCREEN_H

#include "XBase.h"

class XScreen : public XBase { };

// screen cannot be opened
class XScreenOpenFailure : public XScreen {
protected:
	virtual CString		getWhat() const throw();
};

#endif
