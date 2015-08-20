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

#include "base/Event.h"
#include "common/IInterface.h"

//! Screen saver interface
/*!
This interface defines the methods common to all screen savers.
*/
class IScreenSaver : public IInterface {
public:
	// note -- the c'tor/d'tor must *not* enable/disable the screen saver

	//! @name manipulators
	//@{

	//! Enable screen saver
	/*!
	Enable the screen saver, restoring the screen saver settings to
	what they were when disable() was previously called.  If disable()
	wasn't previously called then it should keep the current settings
	or use reasonable defaults.
	*/
	virtual void		enable() = 0;

	//! Disable screen saver
	/*!
	Disable the screen saver, saving the old settings for the next
	call to enable().
	*/
	virtual void		disable() = 0;

	//! Activate screen saver
	/*!
	Activate (i.e. show) the screen saver.
	*/
	virtual void		activate() = 0;

	//! Deactivate screen saver
	/*!
	Deactivate (i.e. hide) the screen saver, reseting the screen saver
	timer.
	*/
	virtual void		deactivate() = 0;

	//@}
	//! @name accessors
	//@{

	//! Test if screen saver on
	/*!
	Returns true iff the screen saver is currently active (showing).
	*/
	virtual bool		isActive() const = 0;

	//@}
};
