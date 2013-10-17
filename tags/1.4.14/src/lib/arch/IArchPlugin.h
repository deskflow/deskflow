/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#pragma once

#define PLUGINS_DIR "plugins"

#include "IInterface.h"

class IEventQueue;

//! Interface for plugin manager.
/*!
A plugin manager should load all 3rd party plugins from the plugins dir,
and then look for common function names in the plugins.
*/
class IArchPlugin : public IInterface {
public:	
	//! @name manipulators
	//@{

	//! Load plugins
	/*!
	Scan the plugins dir and load plugins.
	*/
	virtual void		init(void* eventTarget, IEventQueue* events) = 0;

	//@}
};
