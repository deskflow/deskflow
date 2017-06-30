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

#include "synergy/key_types.h"
#include "synergy/mouse_types.h"
#include "base/Event.h"
#include "base/EventTypes.h"
#include "common/IInterface.h"

//! Primary screen interface
/*!
This interface defines the methods common to all platform dependent
primary screen implementations.
*/
class IPrimaryScreen : public IInterface {
public:
    //! Button event data
    class ButtonInfo {
    public:
        static ButtonInfo* alloc (ButtonID, KeyModifierMask);
        static ButtonInfo* alloc (const ButtonInfo&);

        static bool equal (const ButtonInfo*, const ButtonInfo*);

    public:
        ButtonID m_button;
        KeyModifierMask m_mask;
    };
    //! Motion event data
    class MotionInfo {
    public:
        static MotionInfo* alloc (SInt32 x, SInt32 y);

    public:
        SInt32 m_x;
        SInt32 m_y;
    };
    //! Wheel motion event data
    class WheelInfo {
    public:
        static WheelInfo* alloc (SInt32 xDelta, SInt32 yDelta);

    public:
        SInt32 m_xDelta;
        SInt32 m_yDelta;
    };
    //! Hot key event data
    class HotKeyInfo {
    public:
        static HotKeyInfo* alloc (UInt32 id);

    public:
        UInt32 m_id;
    };

    //! @name manipulators
    //@{

    //! Update configuration
    /*!
    This is called when the configuration has changed.  \c activeSides
    is a bitmask of EDirectionMask indicating which sides of the
    primary screen are linked to clients.  Override to handle the
    possible change in jump zones.
    */
    virtual void reconfigure (UInt32 activeSides) = 0;

    //! Warp cursor
    /*!
    Warp the cursor to the absolute coordinates \c x,y.  Also
    discard input events up to and including the warp before
    returning.
    */
    virtual void warpCursor (SInt32 x, SInt32 y) = 0;

    //! Register a system hotkey
    /*!
    Registers a system-wide hotkey.  The screen should arrange for an event
    to be delivered to itself when the hot key is pressed or released.  When
    that happens the screen should post a \c getHotKeyDownEvent() or
    \c getHotKeyUpEvent(), respectively.  The hot key is key \p key with
    exactly the modifiers \p mask.  Returns 0 on failure otherwise an id
    that can be used to unregister the hotkey.

    A hot key is a set of modifiers and a key, which may itself be a modifier.
    The hot key is pressed when the hot key's modifiers and only those
    modifiers are logically down (active) and the key is pressed.  The hot
    key is released when the key is released, regardless of the modifiers.

    The hot key event should be generated no matter what window or application
    has the focus.  No other window or application should receive the key
    press or release events (they can and should see the modifier key events).
    When the key is a modifier, it's acceptable to allow the user to press
    the modifiers in any order or to require the user to press the given key
    last.
    */
    virtual UInt32 registerHotKey (KeyID key, KeyModifierMask mask) = 0;

    //! Unregister a system hotkey
    /*!
    Unregisters a previously registered hot key.
    */
    virtual void unregisterHotKey (UInt32 id) = 0;

    //! Prepare to synthesize input on primary screen
    /*!
    Prepares the primary screen to receive synthesized input.  We do not
    want to receive this synthesized input as user input so this method
    ensures that we ignore it.  Calls to \c fakeInputBegin() may not be
    nested.
    */
    virtual void fakeInputBegin () = 0;

    //! Done synthesizing input on primary screen
    /*!
    Undoes whatever \c fakeInputBegin() did.
    */
    virtual void fakeInputEnd () = 0;

    //@}
    //! @name accessors
    //@{

    //! Get jump zone size
    /*!
    Return the jump zone size, the size of the regions on the edges of
    the screen that cause the cursor to jump to another screen.
    */
    virtual SInt32 getJumpZoneSize () const = 0;

    //! Test if mouse is pressed
    /*!
    Return true if any mouse button is currently pressed.  Ideally,
    "current" means up to the last processed event but it can mean
    the current physical mouse button state.
    */
    virtual bool isAnyMouseButtonDown (UInt32& buttonID) const = 0;

    //! Get cursor center position
    /*!
    Return the cursor center position which is where we park the
    cursor to compute cursor motion deltas and should be far from
    the edges of the screen, typically the center.
    */
    virtual void getCursorCenter (SInt32& x, SInt32& y) const = 0;

    //@}
};
