/*
 * barrier -- mouse and keyboard sharing utility
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

#include "barrier/KeyState.h"
#include "common/stdmap.h"
#include "common/stdvector.h"
#include "XWindowsImpl.h"

#if X_DISPLAY_MISSING
#    error X11 is required to build barrier
#else
#    include <X11/Xlib.h>
#    if HAVE_X11_EXTENSIONS_XTEST_H
#        include <X11/extensions/XTest.h>
#    else
#        error The XTest extension is required to build barrier
#    endif
#    if HAVE_XKB_EXTENSION
#        include <X11/extensions/XKBstr.h>
#    endif
#endif

class IEventQueue;

//! X Windows key state
/*!
A key state for X Windows.
*/
class XWindowsKeyState : public KeyState {
public:
    typedef std::vector<int> KeycodeList;
    enum {
        kGroupPoll       = -1,
        kGroupPollAndSet = -2
    };

    XWindowsKeyState(IXWindowsImpl* impl, Display*, bool useXKB,
                     IEventQueue* events);
    XWindowsKeyState(IXWindowsImpl* impl, Display*, bool useXKB,
                     IEventQueue* events, barrier::KeyMap& keyMap);
    ~XWindowsKeyState();

    //! @name modifiers
    //@{

    //! Set active group
    /*!
    Sets the active group to \p group.  This is the group returned by
    \c pollActiveGroup().  If \p group is \c kGroupPoll then
    \c pollActiveGroup() will really poll, but that's a slow operation
    on X11.  If \p group is \c kGroupPollAndSet then this will poll the
    active group now and use it for future calls to \c pollActiveGroup().
    */
    void                setActiveGroup(SInt32 group);

    //! Set the auto-repeat state
    /*!
    Sets the auto-repeat state.
    */
    void                setAutoRepeat(const XKeyboardState&);

    //@}
    //! @name accessors
    //@{

    //! Convert X modifier mask to barrier mask
    /*!
    Returns the barrier modifier mask corresponding to the X modifier
    mask in \p state.
    */
    KeyModifierMask        mapModifiersFromX(unsigned int state) const;

    //! Convert barrier modifier mask to X mask
    /*!
    Converts the barrier modifier mask to the corresponding X modifier
    mask.  Returns \c true if successful and \c false if any modifier
    could not be converted.
    */
    bool                mapModifiersToX(KeyModifierMask, unsigned int&) const;

    //! Convert barrier key to all corresponding X keycodes
    /*!
    Converts the barrier key \p key to all of the keycodes that map to
    that key.
    */
    void                mapKeyToKeycodes(KeyID key,
                            KeycodeList& keycodes) const;

    //@}

    // IKeyState overrides
    virtual bool        fakeCtrlAltDel();
    virtual KeyModifierMask
                        pollActiveModifiers() const;
    virtual SInt32        pollActiveGroup() const;
    virtual void        pollPressedKeys(KeyButtonSet& pressedKeys) const;

protected:
    // KeyState overrides
    virtual void        getKeyMap(barrier::KeyMap& keyMap);
    virtual void        fakeKey(const Keystroke& keystroke);

private:
    void                init(Display* display, bool useXKB);
    void                updateKeysymMap(barrier::KeyMap&);
    void                updateKeysymMapXKB(barrier::KeyMap&);
    bool                hasModifiersXKB() const;
    int                    getEffectiveGroup(KeyCode, int group) const;
    UInt32                getGroupFromState(unsigned int state) const;

    static void            remapKeyModifiers(KeyID, SInt32,
                            barrier::KeyMap::KeyItem&, void*);

private:
    struct XKBModifierInfo {
    public:
        unsigned char    m_level;
        UInt32            m_mask;
        bool            m_lock;
    };

#ifdef TEST_ENV
public: // yuck
#endif
    typedef std::vector<KeyModifierMask> KeyModifierMaskList;

private:
    typedef std::map<KeyModifierMask, unsigned int> KeyModifierToXMask;
    typedef std::multimap<KeyID, KeyCode> KeyToKeyCodeMap;
    typedef std::map<KeyCode, unsigned int> NonXKBModifierMap;
    typedef std::map<UInt32, XKBModifierInfo> XKBModifierMap;

    IXWindowsImpl* m_impl;

    Display*            m_display;
#if HAVE_XKB_EXTENSION
    XkbDescPtr            m_xkb;
#endif
    SInt32                m_group;
    XKBModifierMap        m_lastGoodXKBModifiers;
    NonXKBModifierMap    m_lastGoodNonXKBModifiers;

    // X modifier (bit number) to barrier modifier (mask) mapping
    KeyModifierMaskList    m_modifierFromX;

    // barrier modifier (mask) to X modifier (mask)
    KeyModifierToXMask    m_modifierToX;

    // map KeyID to all keycodes that can synthesize that KeyID
    KeyToKeyCodeMap        m_keyCodeFromKey;

    // autorepeat state
    XKeyboardState        m_keyboardState;

#ifdef TEST_ENV
public:
    SInt32                  group() const { return m_group; }
    void                    group(const SInt32& group) { m_group = group; }
    KeyModifierMaskList    modifierFromX() const { return m_modifierFromX; }
#endif
};
