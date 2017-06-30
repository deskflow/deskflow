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

#include "synergy/mouse_types.h"
#include "base/Event.h"
#include "base/EventTypes.h"
#include "common/IInterface.h"

//! Secondary screen interface
/*!
This interface defines the methods common to all platform dependent
secondary screen implementations.
*/
class ISecondaryScreen : public IInterface {
public:
    //! @name accessors
    //@{

    //! Fake mouse press/release
    /*!
    Synthesize a press or release of mouse button \c id.
    */
    virtual void fakeMouseButton (ButtonID id, bool press) = 0;

    //! Fake mouse move
    /*!
    Synthesize a mouse move to the absolute coordinates \c x,y.
    */
    virtual void fakeMouseMove (SInt32 x, SInt32 y) = 0;

    //! Fake mouse move
    /*!
    Synthesize a mouse move to the relative coordinates \c dx,dy.
    */
    virtual void fakeMouseRelativeMove (SInt32 dx, SInt32 dy) const = 0;

    //! Fake mouse wheel
    /*!
    Synthesize a mouse wheel event of amount \c xDelta and \c yDelta.
    */
    virtual void fakeMouseWheel (SInt32 xDelta, SInt32 yDelta) const = 0;

    //@}
};
