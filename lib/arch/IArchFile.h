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

#ifndef IARCHFILE_H
#define IARCHFILE_H

#include "IInterface.h"
#include "stdstring.h"

//! Interface for architecture dependent file system operations
/*!
This interface defines the file system operations required by
synergy.  Each architecture must implement this interface.
*/
class IArchFile : public IInterface {
public:
	//! @name manipulators
	//@{

	//! Extract base name
	/*!
	Find the base name in the given \c pathname.
	*/
	virtual const char*	getBasename(const char* pathname) = 0;

	//! Get user's home directory
	/*!
	Returns the user's home directory.  Returns the empty string if
	this cannot be determined.
	*/
	virtual std::string	getUserDirectory() = 0;

	//! Get system directory
	/*!
	Returns the ussystem configuration file directory.
	*/
	virtual std::string	getSystemDirectory() = 0;

	//! Concatenate path components
	/*!
	Concatenate pathname components with a directory separator
	between them.  This should not check if the resulting path
	is longer than allowed by the system;  we'll rely on the
	system calls to tell us that.
	*/
	virtual std::string	concatPath(
							const std::string& prefix,
							const std::string& suffix) = 0;

	//@}
};

#endif
