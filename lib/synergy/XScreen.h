/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef XSCREEN_H
#define XSCREEN_H

#include "XBase.h"

//! Generic screen exception
XBASE_SUBCLASS(XScreen, XBase);

//! Cannot open screen exception
/*!
Thrown when a screen cannot be opened or initialized.
*/
XBASE_SUBCLASS_WHAT(XScreenOpenFailure, XScreen);

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
