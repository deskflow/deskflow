/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2003 Chris Schoeneman
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

#include "base/String.h"
#include "common/IInterface.h"

class IScreen;
class INode;

//! Interface for architecture dependent task bar event handling
/*!
This interface defines the task bar icon event handlers required
by synergy.  Each architecture must implement this interface
though each operation can be a no-op.
*/
class IArchTaskBarReceiver : public IInterface {
public:
	// Icon data is architecture dependent
	typedef void* Icon;

	//! @name manipulators
	//@{

	//! Show status window
	/*!
	Open a window displaying current status.  This should return
	immediately without waiting for the window to be closed.
	*/
	virtual void		showStatus() = 0;

	//! Popup menu
	/*!
	Popup a menu of operations at or around \c x,y and perform the
	chosen operation.
	*/
	virtual void		runMenu(int x, int y) = 0;

	//! Perform primary action
	/*!
	Perform the primary (default) action.
	*/
	virtual void		primaryAction() = 0;

	//@}
	//! @name accessors
	//@{

	//! Lock receiver
	/*!
	Locks the receiver from changing state.  The receiver should be
	locked when querying it's state to ensure consistent results.
	Each call to \c lock() must have a matching \c unlock() and
	locks cannot be nested.
	*/
	virtual void		lock() const = 0;

	//! Unlock receiver
	virtual void		unlock() const = 0;

	//! Get icon
	/*!
	Returns the icon to display in the task bar.  The interface
	to set the icon is left to subclasses.  Getting and setting
	the icon must be thread safe.
	*/
	virtual const Icon	getIcon() const = 0;

	//! Get tooltip
	/*!
	Returns the tool tip to display in the task bar.  The interface
	to set the tooltip is left to sublclasses.  Getting and setting
	the icon must be thread safe.
	*/
	virtual std::string	getToolTip() const = 0;

	virtual void updateStatus(INode*, const String& errorMsg) = 0;

	virtual void cleanup() {}

	//@}
};
