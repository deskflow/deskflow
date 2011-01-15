/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ISOCKETFACTORY_H
#define ISOCKETFACTORY_H

#include "IInterface.h"

class IDataSocket;
class IListenSocket;

//! Socket factory
/*!
This interface defines the methods common to all factories used to
create sockets.
*/
class ISocketFactory : public IInterface {
public:
	//! @name accessors
	//@{

	//! Create data socket
	virtual IDataSocket*	create() const = 0;

	//! Create listen socket
	virtual IListenSocket*	createListen() const = 0;

	//@}
};

#endif
