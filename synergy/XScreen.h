#ifndef XSCREEN_H
#define XSCREEN_H

#include "XBase.h"

//! Generic screen exception
class XScreen : public XBase { };

//! Cannot open screen exception
/*!
Thrown when a screen cannot be opened or initialized.
*/
class XScreenOpenFailure : public XScreen {
protected:
	virtual CString		getWhat() const throw();
};

#endif
