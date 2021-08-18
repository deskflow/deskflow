/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "synergy/KeyState.h"
#include "common/stdmap.h"
#include "common/stdset.h"
#include "common/stdvector.h"

#include <Carbon/Carbon.h>

class IOSXKeyResource;


//! OS X key state
/*!
A key state for OS X.
*/
class OSXKeyState : public KeyState {
public:
    typedef std::vector<KeyID> KeyIDs;

    OSXKeyState(IEventQueue* events);
    OSXKeyState(IEventQueue* events, synergy::KeyMap& keyMap);
    virtual ~OSXKeyState();

    //! @name modifiers
    //@{

    //! Handle modifier key change
    /*!
    Determines which modifier keys have changed and updates the modifier
    state and sends key events as appropriate.
    */
    void                handleModifierKeys(void* target,
                            KeyModifierMask oldMask, KeyModifierMask newMask);

    //@}
    //! @name accessors
    //@{

    //! Convert OS X modifier mask to synergy mask
    /*!
    Returns the synergy modifier mask corresponding to the OS X modifier
    mask in \p mask.
    */
    KeyModifierMask        mapModifiersFromOSX(UInt32 mask) const;

    //! Convert CG flags-style modifier mask to old-style Carbon
    /*!
    Still required in a few places for translation calls.
    */
    KeyModifierMask        mapModifiersToCarbon(UInt32 mask) const;
    
    //! Map key event to keys
    /*!
    Converts a key event into a sequence of KeyIDs and the shadow modifier
    state to a modifier mask.  The KeyIDs list, in order, the characters
    generated by the key press/release.  It returns the id of the button
    that was pressed or released, or 0 if the button doesn't map to a known
    KeyID.
    */
    KeyButton            mapKeyFromEvent(KeyIDs& ids,
                            KeyModifierMask* maskOut, CGEventRef event) const;

    //! Map key and mask to native values
    /*!
    Calculates mac virtual key and mask for a key \p key and modifiers
    \p mask.  Returns \c true if the key can be mapped, \c false otherwise.
    */
    bool                mapSynergyHotKeyToMac(KeyID key, KeyModifierMask mask,
                            UInt32& macVirtualKey,
                            UInt32& macModifierMask) const;

    //@}

    // IKeyState overrides
    virtual bool        fakeCtrlAltDel();
    virtual bool        fakeMediaKey(KeyID id);
    virtual KeyModifierMask
                        pollActiveModifiers() const;
    virtual SInt32        pollActiveGroup() const;
    virtual void        pollPressedKeys(KeyButtonSet& pressedKeys) const;

    CGEventFlags getModifierStateAsOSXFlags();
protected:
    // KeyState overrides
    virtual void        getKeyMap(synergy::KeyMap& keyMap);
    virtual void        fakeKey(const Keystroke& keystroke);

private:
    class KeyResource;
    typedef void(*CFDeallocator)(CFTypeRef);
    typedef std::unique_ptr<const __CFArray, CFDeallocator> GroupList;
    typedef std::unique_ptr<const __CFDictionary, CFDeallocator> AutoCFDictionary;
    typedef std::unique_ptr<__TISInputSource, CFDeallocator> AutoTISInputSourceRef;


    // Add hard coded special keys to a synergy::KeyMap.
    void                getKeyMapForSpecialKeys(
                            synergy::KeyMap& keyMap, SInt32 group) const;

    // Convert keyboard resource to a key map
    bool                getKeyMap(synergy::KeyMap& keyMap,
                            SInt32 group, const IOSXKeyResource& r) const;

    // Get the available keyboard groups
    bool                getGroups(GroupList&) const;

    // Change active keyboard group to group
    void                setGroup(SInt32 group);

    // Send an event for the given modifier key
    void                handleModifierKey(void* target,
                            UInt32 virtualKey, KeyID id,
                            bool down, KeyModifierMask newMask);

    // Checks if any in \p ids is a glyph key and if \p isCommand is false.
    // If so it adds the AltGr modifier to \p mask.  This allows OS X
    // servers to use the option key both as AltGr and as a modifier.  If
    // option is acting as AltGr (i.e. it generates a glyph and there are
    // no command modifiers active) then we don't send the super modifier
    // to clients because they'd try to match it as a command modifier.
    void                adjustAltGrModifier(const KeyIDs& ids,
                            KeyModifierMask* mask, bool isCommand) const;

    // Maps an OS X virtual key id to a KeyButton.  This simply remaps
    // the ids so we don't use KeyButton 0.
    static KeyButton    mapVirtualKeyToKeyButton(UInt32 keyCode);

    // Maps a KeyButton to an OS X key code.  This is the inverse of
    // mapVirtualKeyToKeyButton.
    static UInt32        mapKeyButtonToVirtualKey(KeyButton keyButton);

    void                init();

    // Get keyboard event flags accorfing to keyboard modifiers
    CGEventFlags getKeyboardEventFlags();

    void setKeyboardModifiers(CGKeyCode virtualKey, bool keyDown);

    void postKeyboardKey(CGKeyCode virtualKey, bool keyDown);

private:
    // OS X uses a physical key if 0 for the 'A' key.  synergy reserves
    // KeyButton 0 so we offset all OS X physical key ids by this much
    // when used as a KeyButton and by minus this much to map a KeyButton
    // to a physical button.
    enum {
        KeyButtonOffset = 1
    };

    typedef std::map<CFDataRef, SInt32> GroupMap;
    typedef std::map<UInt32, KeyID> VirtualKeyMap;

    VirtualKeyMap        m_virtualKeyMap;
    mutable UInt32        m_deadKeyState;
    GroupList           m_groups{nullptr, CFRelease};
    GroupMap            m_groupMap;
    bool                m_shiftPressed;
    bool                m_controlPressed;
    bool                m_altPressed;
    bool                m_superPressed;
    bool                m_capsPressed;
};
