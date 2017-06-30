/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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
 */

#pragma once

#include "common/IInterface.h"

class IArchTaskBarReceiver;

//! Interface for architecture dependent task bar control
/*!
This interface defines the task bar icon operations required
by synergy.  Each architecture must implement this interface
though each operation can be a no-op.
*/
class IArchTaskBar : public IInterface {
public:
    //! @name manipulators
    //@{

    //! Add a receiver
    /*!
    Add a receiver object to be notified of user and application
    events.  This should be called before other methods.  When
    the receiver is added to the task bar, its icon appears on
    the task bar.
    */
    virtual void addReceiver (IArchTaskBarReceiver*) = 0;

    //! Remove a receiver
    /*!
    Remove a receiver object from the task bar.  This removes the
    icon from the task bar.
    */
    virtual void removeReceiver (IArchTaskBarReceiver*) = 0;

    //! Update a receiver
    /*!
    Updates the display of the receiver on the task bar.  This
    should be called when the receiver appearance may have changed
    (e.g. it's icon or tool tip has changed).
    */
    virtual void updateReceiver (IArchTaskBarReceiver*) = 0;

    //@}

    virtual void init () = 0;
};
