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

#ifndef IARCHCONSOLE_H
#define IARCHCONSOLE_H

#include "IInterface.h"

//! Interface for architecture dependent console output
/*!
This interface defines the console operations required by
synergy.  Each architecture must implement this interface.
*/
class IArchConsole : public IInterface {
public:
	//! @name manipulators
	//@{

	//! Open the console
	/*!
	Opens the console for writing.  The console is opened automatically
	on the first write so calling this method is optional.  Uses \c title
	for the console's title if appropriate for the architecture.  Calling
	this method on an already open console must have no effect.
	*/
	virtual void		openConsole(const char* title) = 0;

	//! Close the console
	/*!
	Close the console.  Calling this method on an already closed console
	must have no effect.
	*/
	virtual void		closeConsole() = 0;

	//! Write to the console
	/*!
	Writes the given string to the console, opening it if necessary.
	*/
	virtual void		writeConsole(const char*) = 0;

	//! Returns the newline sequence for the console
	/*!
	Different consoles use different character sequences for newlines.
	This method returns the appropriate newline sequence for the console.
	*/
	virtual const char*	getNewlineForConsole() = 0;

	//@}
};

#endif
