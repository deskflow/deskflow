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

#ifndef IARCHSTRING_H
#define IARCHSTRING_H

#include "IInterface.h"
#include <stdarg.h>

class CArchMBStateImpl;
typedef CArchMBStateImpl* CArchMBState;

class IArchString : public IInterface {
public:
	//! @name manipulators
	//@{

	//! printf() to limited size buffer with va_list
	/*!
	This method is equivalent to vsprintf() except it will not write
	more than \c n bytes to the buffer, returning -1 if the output
    was truncated and the number of bytes written not including the
    trailing NUL otherwise.
	*/
	virtual int			vsnprintf(char* str,
							int size, const char* fmt, va_list ap) = 0;

	//! Create a new multibyte conversion state
	virtual CArchMBState	newMBState() = 0;

	//! Destroy a multibyte conversion state
	virtual void		closeMBState(CArchMBState) = 0;

	//! Initialize a multibyte conversion state
	virtual void		initMBState(CArchMBState) = 0;

	//! Test a multibyte conversion state
	virtual bool		isInitMBState(CArchMBState) = 0;

	//! Convert multibyte to wide character
	virtual int			convMBToWC(wchar_t*,
							const char*, int, CArchMBState) = 0;

	//! Convert wide character to multibyte
	virtual int			convWCToMB(char*, wchar_t, CArchMBState) = 0;

	//@}
};

#endif
