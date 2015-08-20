/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2004 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#pragma once

#include "common/IInterface.h"
#include "common/stdstring.h"

//! Interface for architecture dependent system queries
/*!
This interface defines operations for querying system info.
*/
class IArchSystem : public IInterface {
public:
	//! @name accessors
	//@{

	//! Identify the OS
	/*!
	Returns a string identifying the operating system.
	*/
	virtual std::string	getOSName() const = 0;

	//! Identify the platform
	/*!
	Returns a string identifying the platform this OS is running on.
	*/
	virtual std::string getPlatformName() const = 0;
	//@}

	//! Get a Synergy setting
	/*!
	Reads a Synergy setting from the system.
	*/
	virtual std::string setting(const std::string& valueName) const = 0;
	//@}

	//! Set a Synergy setting
	/*!
	Writes a Synergy setting from the system.
	*/
	virtual void setting(const std::string& valueName, const std::string& valueString) const = 0;
	//@}

	//! Get the pathnames of the libraries used by Synergy
	/*
	Returns a string containing the full path names of all loaded libraries at the point it is called.
	*/
	virtual std::string getLibsUsed(void) const = 0;
	//@}
};
