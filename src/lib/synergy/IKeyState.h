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
#include "base/Event.h"
#include "base/String.h"
#include "base/IEventQueue.h"
#include "base/EventTypes.h"
#include "common/stdset.h"
#include "common/IInterface.h"

//! Key state interface
/*!
This interface provides access to set and query the keyboard state and
to synthesize key events.
*/
class IKeyState : public IInterface {
public:
    IKeyState (IEventQueue* events);

    enum { kNumButtons = 0x200 };

    //! Key event data
    class KeyInfo {
    public:
        static KeyInfo* alloc (KeyID, KeyModifierMask, KeyButton, SInt32 count);
        static KeyInfo* alloc (KeyID, KeyModifierMask, KeyButton, SInt32 count,
                               const std::set<String>& destinations);
        static KeyInfo* alloc (const KeyInfo&);

        static bool isDefault (const char* screens);
        static bool contains (const char* screens, const String& name);
        static bool equal (const KeyInfo*, const KeyInfo*);
        static String join (const std::set<String>& destinations);
        static void split (const char* screens, std::set<String>&);

    public:
        KeyID m_key;
        KeyModifierMask m_mask;
        KeyButton m_button;
        SInt32 m_count;
        char* m_screens;
        char m_screensBuffer[1];
    };

    typedef std::set<KeyButton> KeyButtonSet;

    //! @name manipulators
    //@{

    //! Update the keyboard map
    /*!
    Causes the key state to get updated to reflect the current keyboard
    mapping.
    */
    virtual void updateKeyMap () = 0;

    //! Update the key state
    /*!
    Causes the key state to get updated to reflect the physical keyboard
    state.
    */
    virtual void updateKeyState () = 0;

    //! Set half-duplex mask
    /*!
    Sets which modifier toggle keys are half-duplex.  A half-duplex
    toggle key doesn't report a key release when toggled on and
    doesn't report a key press when toggled off.
    */
    virtual void setHalfDuplexMask (KeyModifierMask) = 0;

    //! Fake a key press
    /*!
    Synthesizes a key press event and updates the key state.
    */
    virtual void
    fakeKeyDown (KeyID id, KeyModifierMask mask, KeyButton button) = 0;

    //! Fake a key repeat
    /*!
    Synthesizes a key repeat event and updates the key state.
    */
    virtual bool fakeKeyRepeat (KeyID id, KeyModifierMask mask, SInt32 count,
                                KeyButton button) = 0;

    //! Fake a key release
    /*!
    Synthesizes a key release event and updates the key state.
    */
    virtual bool fakeKeyUp (KeyButton button) = 0;

    //! Fake key releases for all fake pressed keys
    /*!
    Synthesizes a key release event for every key that is synthetically
    pressed and updates the key state.
    */
    virtual void fakeAllKeysUp () = 0;

    //! Fake ctrl+alt+del
    /*!
    Synthesize a press of ctrl+alt+del.  Return true if processing is
    complete and false if normal key processing should continue.
    */
    virtual bool fakeCtrlAltDel () = 0;

    //! Fake a media key
    /*!
     Synthesizes a media key down and up. Only Mac would implement this by
     use cocoa appkit framework.
     */
    virtual bool fakeMediaKey (KeyID id) = 0;

    //@}
    //! @name accessors
    //@{

    //! Test if key is pressed
    /*!
    Returns true iff the given key is down.  Half-duplex toggles
    always return false.
    */
    virtual bool isKeyDown (KeyButton) const = 0;

    //! Get the active modifiers
    /*!
    Returns the modifiers that are currently active according to our
    shadowed state.
    */
    virtual KeyModifierMask getActiveModifiers () const = 0;

    //! Get the active modifiers from OS
    /*!
    Returns the modifiers that are currently active according to the
    operating system.
    */
    virtual KeyModifierMask pollActiveModifiers () const = 0;

    //! Get the active keyboard layout from OS
    /*!
    Returns the active keyboard layout according to the operating system.
    */
    virtual SInt32 pollActiveGroup () const = 0;

    //! Get the keys currently pressed from OS
    /*!
    Adds any keys that are currently pressed according to the operating
    system to \p pressedKeys.
    */
    virtual void pollPressedKeys (KeyButtonSet& pressedKeys) const = 0;

    //@}
};
