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

//! Screen unavailable exception
/*!
Thrown when a screen cannot be opened or initialized but retrying later
may be successful.
*/
class XScreenUnavailable : public XScreenOpenFailure {
public:
	/*!
	\c timeUntilRetry is the suggested time the caller should wait until
	trying to open the screen again.
	*/
	XScreenUnavailable(double timeUntilRetry);
	virtual ~XScreenUnavailable();

	//! @name manipulators
	//@{

	//! Get retry time
	/*!
	Returns the suggested time to wait until retrying to open the screen.
	*/
	double				getRetryTime() const;

	//@}

protected:
	virtual CString		getWhat() const throw();

private:
	double				m_timeUntilRetry;
};

#endif
