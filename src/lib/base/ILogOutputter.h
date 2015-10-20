/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "base/Log.h"
#include "base/ELevel.h"
#include "common/IInterface.h"

//! Outputter interface
/*!
Type of outputter interface.  The logger performs all output through
outputters.  ILogOutputter overrides must not call any log functions
directly or indirectly.
*/
class ILogOutputter : public IInterface {
public:
	//! @name manipulators
	//@{

	//! Open the outputter
	/*!
	Opens the outputter for writing.  Calling this method on an
	already open outputter must have no effect.
	*/
	virtual void		open(const char* title) = 0;

	//! Close the outputter
	/*!
	Close the outputter.  Calling this method on an already closed
	outputter must have no effect.
	*/
	virtual void		close() = 0;

	//! Show the outputter
	/*!
	Causes the output to become visible.  This generally only makes sense
	for a logger in a graphical user interface.  Other implementations
	will do nothing.  Iff \p showIfEmpty is \c false then the implementation
	may optionally only show the log if it's not empty.
	*/
	virtual void		show(bool showIfEmpty) = 0;

	//! Write a message with level
	/*!
	Writes \c message, which has the given \c level, to a log.
	If this method returns true then Log will stop passing the
	message to all outputters in the outputter chain, otherwise
	it continues.  Most implementations should return true.
	*/
	virtual bool		write(ELevel level, const char* message) = 0;

	//@}
};
