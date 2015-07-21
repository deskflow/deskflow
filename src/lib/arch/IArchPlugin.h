/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2012 Nick Bolton
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
 */

#pragma once

#include "common/IInterface.h"
#include "common/stdmap.h"
#include "base/String.h"

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

	//!Load plugins
	/*!
	Scan the plugins dir and load plugins.
	*/
	virtual void		load() = 0;

	//!Unload plugins
	/*!
	Look through the loaded plugins and unload them.
	*/
	virtual void		unload() = 0;

	//! Init the common parts
	/*!
	Initializes common parts like log and arch.
	*/
	virtual void		init(void* log, void* arch) = 0;

	//! Init the event part
	/*!
	Initializes event parts.
	*/
	virtual void		initEvent(void* eventTarget, IEventQueue* events) = 0;

	//! Check if exists
	/*!
	Returns true if the plugin exists and is loaded.
	*/
	virtual bool		exists(const char* name) = 0;

	//! Invoke function
	/*!
	Invokes a function from the plugin.
	*/
	virtual void*		invoke(const char* plugin,
							const char* command,
							void** args,
							void* library = NULL) = 0;

	//@}

protected:
	typedef std::map<String, void*> PluginTable;
};
