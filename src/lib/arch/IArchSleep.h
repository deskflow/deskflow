/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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
 */

#pragma once

#include "common/IInterface.h"

//! Interface for architecture dependent sleeping
/*!
This interface defines the sleep operations required by
synergy.  Each architecture must implement this interface.
*/
class IArchSleep : public IInterface {
public:
    //! @name manipulators
    //@{

    //! Sleep
    /*!
    Blocks the calling thread for \c timeout seconds.  If
    \c timeout < 0.0 then the call returns immediately.  If \c timeout
    == 0.0 then the calling thread yields the CPU.

    (cancellation point)
    */
    virtual void sleep (double timeout) = 0;

    //@}
};
