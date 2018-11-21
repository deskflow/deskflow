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

#include "platform/XWindowsKeyState.h"

#include "platform/XWindowsUtil.h"
#include "base/Log.h"
#include "base/String.h"
#include "common/stdmap.h"

#include <cstddef>
#include <algorithm>
#if X_DISPLAY_MISSING
#    error X11 is required to build barrier
#else
#    include <X11/X.h>
#    include <X11/Xutil.h>
#    define XK_MISCELLANY
#    define XK_XKB_KEYS
#    include <X11/keysymdef.h>
#if HAVE_XKB_EXTENSION
#    include <X11/XKBlib.h>
#endif
#endif

static const size_t ModifiersFromXDefaultSize = 32;

XWindowsKeyState::XWindowsKeyState(IXWindowsImpl* impl,
        Display* display, bool useXKB,
        IEventQueue* events) :
    KeyState(events),
    m_display(display),
    m_modifierFromX(ModifiersFromXDefaultSize)
{
     m_impl = impl;

    init(display, useXKB);
}

XWindowsKeyState::XWindowsKeyState(IXWindowsImpl* impl,
    Display* display, bool useXKB,
    IEventQueue* events, barrier::KeyMap& keyMap) :
    KeyState(events, keyMap),
    m_display(display),
    m_modifierFromX(ModifiersFromXDefaultSize)
{
    m_impl = impl;
    init(display, useXKB);
}

XWindowsKeyState::~XWindowsKeyState()
{
#if HAVE_XKB_EXTENSION
    if (m_xkb != NULL) {
        m_impl->XkbFreeKeyboard(m_xkb, 0, True);
    }
#endif
}

void
XWindowsKeyState::init(Display* display, bool useXKB)
{
    XGetKeyboardControl(m_display, &m_keyboardState);
#if HAVE_XKB_EXTENSION
    if (useXKB) {
        m_xkb = m_impl->XkbGetMap(m_display,
                                  XkbKeyActionsMask | XkbKeyBehaviorsMask |
                                  XkbAllClientInfoMask, XkbUseCoreKbd);
    }
    else {
        m_xkb = NULL;
    }
#endif
    setActiveGroup(kGroupPollAndSet);
}

void
XWindowsKeyState::setActiveGroup(SInt32 group)
{
    if (group == kGroupPollAndSet) {
        // we need to set the group to -1 in order for pollActiveGroup() to
        // actually poll for the group
        m_group = -1;
        m_group = pollActiveGroup();
    }
    else if (group == kGroupPoll) {
        m_group = -1;
    }
    else {
        assert(group >= 0);
        m_group = group;
    }
}

void
XWindowsKeyState::setAutoRepeat(const XKeyboardState& state)
{
    m_keyboardState = state;
}

KeyModifierMask
XWindowsKeyState::mapModifiersFromX(unsigned int state) const
{
    LOG((CLOG_DEBUG2 "mapping state: %i", state));
    UInt32 offset = 8 * getGroupFromState(state);
    KeyModifierMask mask = 0;
    for (int i = 0; i < 8; ++i) {
        if ((state & (1u << i)) != 0) {
            LOG((CLOG_DEBUG2 "|= modifier: %i", offset + i));
            if (offset + i >= m_modifierFromX.size()) {
                LOG((CLOG_ERR "m_modifierFromX is too small (%d) for the "
                    "requested offset (%d)", m_modifierFromX.size(), offset+i));
            } else {
                mask |= m_modifierFromX[offset + i];
            }
        }
    }
    return mask;
}

bool
XWindowsKeyState::mapModifiersToX(KeyModifierMask mask,
                unsigned int& modifiers) const
{
    modifiers = 0;

    for (SInt32 i = 0; i < kKeyModifierNumBits; ++i) {
        KeyModifierMask bit = (1u << i);
        if ((mask & bit) != 0) {
            KeyModifierToXMask::const_iterator j = m_modifierToX.find(bit);
            if (j == m_modifierToX.end()) {
                return false;
            }
            else {
                modifiers |= j->second;
            }
        }
    }

    return true;
}

void
XWindowsKeyState::mapKeyToKeycodes(KeyID key, KeycodeList& keycodes) const
{
    keycodes.clear();
    std::pair<KeyToKeyCodeMap::const_iterator,
        KeyToKeyCodeMap::const_iterator> range =
            m_keyCodeFromKey.equal_range(key);
    for (KeyToKeyCodeMap::const_iterator i = range.first;
                                i != range.second; ++i) {
        keycodes.push_back(i->second);
    }
}

bool
XWindowsKeyState::fakeCtrlAltDel()
{
    // pass keys through unchanged
    return false;
}

KeyModifierMask
XWindowsKeyState::pollActiveModifiers() const
{
    Window root = DefaultRootWindow(m_display), window;
    int xRoot, yRoot, xWindow, yWindow;
    unsigned int state = 0;
    if (m_impl->XQueryPointer(m_display, root, &root, &window,
                              &xRoot, &yRoot, &xWindow, &yWindow, &state
                              ) == False) {
        state = 0;
    }
    return mapModifiersFromX(state);
}

SInt32
XWindowsKeyState::pollActiveGroup() const
{
    // fixed condition where any group < -1 would have undetermined behaviour
    if (m_group >= 0) {
        return m_group;
    }

#if HAVE_XKB_EXTENSION
    if (m_xkb != NULL) {
        XkbStateRec state;
        if (m_impl->XkbGetState(m_display, XkbUseCoreKbd, &state) == Success) {
            return state.group;
        }
    }
#endif
    return 0;
}

void
XWindowsKeyState::pollPressedKeys(KeyButtonSet& pressedKeys) const
{
    char keys[32];
    m_impl->XQueryKeymap(m_display, keys);
    for (UInt32 i = 0; i < 32; ++i) {
        for (UInt32 j = 0; j < 8; ++j) {
            if ((keys[i] & (1u << j)) != 0) {
                pressedKeys.insert(8 * i + j);
            }
        }
    }
}

void
XWindowsKeyState::getKeyMap(barrier::KeyMap& keyMap)
{
    // get autorepeat info.  we must use the global_auto_repeat told to
    // us because it may have modified by barrier.
    int oldGlobalAutoRepeat = m_keyboardState.global_auto_repeat;
    XGetKeyboardControl(m_display, &m_keyboardState);
    m_keyboardState.global_auto_repeat = oldGlobalAutoRepeat;

#if HAVE_XKB_EXTENSION
    if (m_xkb != NULL) {
        unsigned mask = XkbKeyActionsMask | XkbKeyBehaviorsMask |
                        XkbAllClientInfoMask;
        if (m_impl->XkbGetUpdatedMap(m_display, mask, m_xkb) == Success) {
            updateKeysymMapXKB(keyMap);
            return;
        }
    }
#endif
    updateKeysymMap(keyMap);
}

void
XWindowsKeyState::fakeKey(const Keystroke& keystroke)
{
    switch (keystroke.m_type) {
    case Keystroke::kButton:
        LOG((CLOG_DEBUG1 "  %03x (%08x) %s", keystroke.m_data.m_button.m_button, keystroke.m_data.m_button.m_client, keystroke.m_data.m_button.m_press ? "down" : "up"));
        if (keystroke.m_data.m_button.m_repeat) {
            int c = keystroke.m_data.m_button.m_button;
            int i = (c >> 3);
            int b = 1 << (c & 7);
            if (m_keyboardState.global_auto_repeat == AutoRepeatModeOff ||
                (c!=113 && c!=116 && (m_keyboardState.auto_repeats[i] & b) == 0)) {
                LOG((CLOG_DEBUG1 "  discard autorepeat"));
                break;
            }
        }
        m_impl->XTestFakeKeyEvent(m_display, keystroke.m_data.m_button.m_button,
                                  keystroke.m_data.m_button.m_press,
                                  CurrentTime);
        break;

    case Keystroke::kGroup:
        if (keystroke.m_data.m_group.m_absolute) {
            LOG((CLOG_DEBUG1 "  group %d", keystroke.m_data.m_group.m_group));
#if HAVE_XKB_EXTENSION
            if (m_xkb != NULL) {
                if (m_impl->XkbLockGroup(m_display, XkbUseCoreKbd,
                                         keystroke.m_data.m_group.m_group
                                         ) == False) {
                    LOG((CLOG_DEBUG1 "XkbLockGroup request not sent"));
                }
            }
            else
#endif
            {
                LOG((CLOG_DEBUG1 "  ignored"));
            }
        }
        else {
            LOG((CLOG_DEBUG1 "  group %+d", keystroke.m_data.m_group.m_group));
#if HAVE_XKB_EXTENSION
            if (m_xkb != NULL) {
                if (m_impl->XkbLockGroup(m_display, XkbUseCoreKbd,
                                         getEffectiveGroup(pollActiveGroup(),
                                         keystroke.m_data.m_group.m_group)
                                         ) == False) {
                    LOG((CLOG_DEBUG1 "XkbLockGroup request not sent"));
                }
            }
            else
#endif
            {
                LOG((CLOG_DEBUG1 "  ignored"));
            }
        }
        break;
    }
    XFlush(m_display);
}

void
XWindowsKeyState::updateKeysymMap(barrier::KeyMap& keyMap)
{
    // there are up to 4 keysyms per keycode
    static const int maxKeysyms = 4;

    LOG((CLOG_DEBUG1 "non-XKB mapping"));

    // prepare map from X modifier to KeyModifierMask.  certain bits
    // are predefined.
    std::fill(m_modifierFromX.begin(), m_modifierFromX.end(), 0);
    m_modifierFromX[ShiftMapIndex]   = KeyModifierShift;
    m_modifierFromX[LockMapIndex]    = KeyModifierCapsLock;
    m_modifierFromX[ControlMapIndex] = KeyModifierControl;
    m_modifierToX.clear();
    m_modifierToX[KeyModifierShift]    = ShiftMask;
    m_modifierToX[KeyModifierCapsLock] = LockMask;
    m_modifierToX[KeyModifierControl]  = ControlMask;

    // prepare map from KeyID to KeyCode
    m_keyCodeFromKey.clear();

    // get the number of keycodes
    int minKeycode, maxKeycode;
    m_impl->XDisplayKeycodes(m_display, &minKeycode, &maxKeycode);
    int numKeycodes = maxKeycode - minKeycode + 1;

    // get the keyboard mapping for all keys
    int keysymsPerKeycode;
    KeySym* allKeysyms = m_impl->XGetKeyboardMapping(m_display,
                                                     minKeycode, numKeycodes,
                                                     &keysymsPerKeycode);

    // it's more convenient to always have maxKeysyms KeySyms per key
    {
        KeySym* tmpKeysyms = new KeySym[maxKeysyms * numKeycodes];
        for (int i = 0; i < numKeycodes; ++i) {
            for (int j = 0; j < maxKeysyms; ++j) {
                if (j < keysymsPerKeycode) {
                    tmpKeysyms[maxKeysyms * i + j] =
                        allKeysyms[keysymsPerKeycode * i + j];
                }
                else {
                    tmpKeysyms[maxKeysyms * i + j] = NoSymbol;
                }
            }    
        }
        m_impl->XFree(allKeysyms);
        allKeysyms = tmpKeysyms;
    }

    // get the buttons assigned to modifiers.  X11 does not predefine
    // the meaning of any modifiers except shift, caps lock, and the
    // control key.  the meaning of a modifier bit (other than those)
    // depends entirely on the KeySyms mapped to that bit.  unfortunately
    // you cannot map a bit back to the KeySym used to produce it.
    // for example, let's say button 1 maps to Alt_L without shift and
    // Meta_L with shift.  now if mod1 is mapped to button 1 that could
    // mean the user used Alt or Meta to turn on that modifier and there's
    // no way to know which.  it's also possible for one button to be
    // mapped to multiple bits so both mod1 and mod2 could be generated
    // by button 1.
    //
    // we're going to ignore any modifier for a button except the first.
    // with the above example, that means we'll ignore the mod2 modifier
    // bit unless it's also mapped to some other button.  we're also
    // going to ignore all KeySyms except the first modifier KeySym,
    // which means button 1 above won't map to Meta, just Alt.
    std::map<KeyCode, unsigned int> modifierButtons;
    XModifierKeymap* modifiers = m_impl->XGetModifierMapping(m_display);
    for (unsigned int i = 0; i < 8; ++i) {
        const KeyCode* buttons =
            modifiers->modifiermap + i * modifiers->max_keypermod;
        for (int j = 0; j < modifiers->max_keypermod; ++j) {
            modifierButtons.insert(std::make_pair(buttons[j], i));
        }
    }
    XFreeModifiermap(modifiers);
    modifierButtons.erase(0);

    // Hack to deal with VMware.  When a VMware client grabs input the
    // player clears out the X modifier map for whatever reason.  We're
    // notified of the change and arrive here to discover that there
    // are no modifiers at all.  Since this prevents the modifiers from
    // working in the VMware client we'll use the last known good set
    // of modifiers when there are no modifiers.  If there are modifiers
    // we update the last known good set.
    if (!modifierButtons.empty()) {
        m_lastGoodNonXKBModifiers = modifierButtons;
    }
    else {
        modifierButtons = m_lastGoodNonXKBModifiers;
    }

    // add entries for each keycode
    barrier::KeyMap::KeyItem item;
    for (int i = 0; i < numKeycodes; ++i) {
        KeySym* keysyms = allKeysyms + maxKeysyms * i;
        KeyCode keycode = static_cast<KeyCode>(i + minKeycode);
        item.m_button   = static_cast<KeyButton>(keycode);
        item.m_client   = 0;

        // determine modifier sensitivity
        item.m_sensitive = 0;

        // if the keysyms in levels 2 or 3 exist and differ from levels
        // 0 and 1 then the key is sensitive AltGr (Mode_switch)
        if ((keysyms[2] != NoSymbol && keysyms[2] != keysyms[0]) ||
            (keysyms[3] != NoSymbol && keysyms[2] != keysyms[1])) {
            item.m_sensitive |= KeyModifierAltGr;
        }

        // check if the key is caps-lock sensitive.  some systems only
        // provide one keysym for keys sensitive to caps-lock.  if we
        // find that then fill in the missing keysym.
        if (keysyms[0] != NoSymbol && keysyms[1] == NoSymbol &&
            keysyms[2] == NoSymbol && keysyms[3] == NoSymbol) {
            KeySym lKeysym, uKeysym;
            XConvertCase(keysyms[0], &lKeysym, &uKeysym);
            if (lKeysym != uKeysym) {
                keysyms[0] = lKeysym;
                keysyms[1] = uKeysym;
                item.m_sensitive |= KeyModifierCapsLock;
            }
        }
        else if (keysyms[0] != NoSymbol && keysyms[1] != NoSymbol) {
            KeySym lKeysym, uKeysym;
            XConvertCase(keysyms[0], &lKeysym, &uKeysym);
            if (lKeysym != uKeysym &&
                lKeysym == keysyms[0] &&
                uKeysym == keysyms[1]) {
                item.m_sensitive |= KeyModifierCapsLock;
            }
            else if (keysyms[2] != NoSymbol && keysyms[3] != NoSymbol) {
                XConvertCase(keysyms[2], &lKeysym, &uKeysym);
                if (lKeysym != uKeysym &&
                    lKeysym == keysyms[2] &&
                    uKeysym == keysyms[3]) {
                    item.m_sensitive |= KeyModifierCapsLock;
                }
            }
        }

        // key is sensitive to shift if keysyms in levels 0 and 1 or
        // levels 2 and 3 don't match.  it's also sensitive to shift
        // if it's sensitive to caps-lock.
        if ((item.m_sensitive & KeyModifierCapsLock) != 0) {
            item.m_sensitive |= KeyModifierShift;
        }
        else if ((keysyms[0] != NoSymbol && keysyms[1] != NoSymbol &&
                keysyms[0] != keysyms[1]) ||
                (keysyms[2] != NoSymbol && keysyms[3] != NoSymbol &&
                keysyms[2] != keysyms[3])) {
            item.m_sensitive |= KeyModifierShift;
        }

        // key is sensitive to numlock if any keysym on it is
        if (IsKeypadKey(keysyms[0]) || IsPrivateKeypadKey(keysyms[0]) ||
            IsKeypadKey(keysyms[1]) || IsPrivateKeypadKey(keysyms[1]) ||
            IsKeypadKey(keysyms[2]) || IsPrivateKeypadKey(keysyms[2]) ||
            IsKeypadKey(keysyms[3]) || IsPrivateKeypadKey(keysyms[3])) {
            item.m_sensitive |= KeyModifierNumLock;
        }

        // do each keysym (shift level)
        for (int j = 0; j < maxKeysyms; ++j) {
            item.m_id = XWindowsUtil::mapKeySymToKeyID(keysyms[j]);
            if (item.m_id == kKeyNone) {
                if (j != 0 && modifierButtons.count(keycode) > 0) {
                    // pretend the modifier works in other shift levels
                    // because it probably does.
                    if (keysyms[1] == NoSymbol || j != 3) {
                        item.m_id = XWindowsUtil::mapKeySymToKeyID(keysyms[0]);
                    }
                    else {
                        item.m_id = XWindowsUtil::mapKeySymToKeyID(keysyms[1]);
                    }
                }
                if (item.m_id == kKeyNone) {
                    continue;
                }
            }

            // group is 0 for levels 0 and 1 and 1 for levels 2 and 3
            item.m_group = (j >= 2) ? 1 : 0;

            // compute required modifiers
            item.m_required = 0;
            if ((j & 1) != 0) {
                item.m_required |= KeyModifierShift;
            }
            if ((j & 2) != 0) {
                item.m_required |= KeyModifierAltGr;
            }

            item.m_generates = 0;
            item.m_lock      = false;
            if (modifierButtons.count(keycode) > 0) {
                // get flags for modifier keys
                barrier::KeyMap::initModifierKey(item);

                // add mapping from X (unless we already have)
                if (item.m_generates != 0) {
                    unsigned int bit = modifierButtons[keycode];
                    if (m_modifierFromX[bit] == 0) {
                        m_modifierFromX[bit] = item.m_generates;
                        m_modifierToX[item.m_generates] = (1u << bit);
                    }
                }
            }

            // add key
            keyMap.addKeyEntry(item);
            m_keyCodeFromKey.insert(std::make_pair(item.m_id, keycode));

            // add other ways to synthesize the key
            if ((j & 1) != 0) {
                // add capslock version of key is sensitive to capslock
                KeySym lKeysym, uKeysym;
                XConvertCase(keysyms[j], &lKeysym, &uKeysym);
                if (lKeysym != uKeysym &&
                    lKeysym == keysyms[j - 1] &&
                    uKeysym == keysyms[j]) {
                    item.m_required &= ~KeyModifierShift;
                    item.m_required |=  KeyModifierCapsLock;
                    keyMap.addKeyEntry(item);
                    item.m_required |=  KeyModifierShift;
                    item.m_required &= ~KeyModifierCapsLock;
                }

                // add numlock version of key if sensitive to numlock
                if (IsKeypadKey(keysyms[j]) || IsPrivateKeypadKey(keysyms[j])) {
                    item.m_required &= ~KeyModifierShift;
                    item.m_required |=  KeyModifierNumLock;
                    keyMap.addKeyEntry(item);
                    item.m_required |=  KeyModifierShift;
                    item.m_required &= ~KeyModifierNumLock;
                }
            }
        }
    }

    delete[] allKeysyms;
}

#if HAVE_XKB_EXTENSION
void
XWindowsKeyState::updateKeysymMapXKB(barrier::KeyMap& keyMap)
{
    static const XkbKTMapEntryRec defMapEntry = {
        True,        // active
        0,            // level
        {
            0,        // mods.mask
            0,        // mods.real_mods
            0        // mods.vmods
        }
    };

    LOG((CLOG_DEBUG1 "XKB mapping"));

    // find the number of groups
    int maxNumGroups = 0;
    for (int i = m_xkb->min_key_code; i <= m_xkb->max_key_code; ++i) {
        int numGroups = m_impl->do_XkbKeyNumGroups(m_xkb, static_cast<KeyCode>(i));
        if (numGroups > maxNumGroups) {
            maxNumGroups = numGroups;
        }
    }

    // prepare map from X modifier to KeyModifierMask
    std::vector<int> modifierLevel(maxNumGroups * 8, 4);
    m_modifierFromX.clear();
    m_modifierFromX.resize(maxNumGroups * 8);
    m_modifierToX.clear();

    // prepare map from KeyID to KeyCode
    m_keyCodeFromKey.clear();

    // Hack to deal with VMware.  When a VMware client grabs input the
    // player clears out the X modifier map for whatever reason.  We're
    // notified of the change and arrive here to discover that there
    // are no modifiers at all.  Since this prevents the modifiers from
    // working in the VMware client we'll use the last known good set
    // of modifiers when there are no modifiers.  If there are modifiers
    // we update the last known good set.
    bool useLastGoodModifiers = !hasModifiersXKB();
    if (!useLastGoodModifiers) {
        m_lastGoodXKBModifiers.clear();
    }

    // check every button.  on this pass we save all modifiers as native
    // X modifier masks.
    barrier::KeyMap::KeyItem item;
    for (int i = m_xkb->min_key_code; i <= m_xkb->max_key_code; ++i) {
        KeyCode keycode = static_cast<KeyCode>(i);
        item.m_button   = static_cast<KeyButton>(keycode);
        item.m_client   = 0;

        // skip keys with no groups (they generate no symbols)
        if (m_impl->do_XkbKeyNumGroups(m_xkb, keycode) == 0) {
            continue;
        }

        // note half-duplex keys
        const XkbBehavior& b = m_xkb->server->behaviors[keycode];
        if ((b.type & XkbKB_OpMask) == XkbKB_Lock) {
            keyMap.addHalfDuplexButton(item.m_button);
        }

        // iterate over all groups
        for (int group = 0; group < maxNumGroups; ++group) {
            item.m_group = group;
            int eGroup   = getEffectiveGroup(keycode, group);

            // get key info
            XkbKeyTypePtr type = m_impl->do_XkbKeyKeyType(m_xkb, keycode,
                                                          eGroup);

            // set modifiers the item is sensitive to
            item.m_sensitive = type->mods.mask;

            // iterate over all shift levels for the button (including none)
            for (int j = -1; j < type->map_count; ++j) {
                const XkbKTMapEntryRec* mapEntry =
                    ((j == -1) ? &defMapEntry : type->map + j);
                if (!mapEntry->active) {
                    continue;
                }
                int level = mapEntry->level;

                // set required modifiers for this item
                item.m_required = mapEntry->mods.mask;
                if ((item.m_required & LockMask) != 0 &&
                    j != -1 && type->preserve != NULL &&
                    (type->preserve[j].mask & LockMask) != 0) {
                    // sensitive caps lock and we preserve caps-lock.
                    // preserving caps-lock means we Xlib functions would
                    // yield the capitialized KeySym so we'll adjust the
                    // level accordingly.
                    if ((level ^ 1) < type->num_levels) {
                        level ^= 1;
                    }
                }

                // get the keysym for this item
                KeySym keysym = m_impl->do_XkbKeySymEntry(m_xkb, keycode, level,
                                                          eGroup);

                // check for group change actions, locking modifiers, and
                // modifier masks.
                item.m_lock         = false;
                bool isModifier     = false;
                UInt32 modifierMask = m_xkb->map->modmap[keycode];
                if (m_impl->do_XkbKeyHasActions(m_xkb, keycode) == True) {
                    XkbAction* action =
                        m_impl->do_XkbKeyActionEntry(m_xkb, keycode, level,
                                                     eGroup);
                    if (action->type == XkbSA_SetMods ||
                        action->type == XkbSA_LockMods) {
                        isModifier  = true;

                        // note toggles
                        item.m_lock = (action->type == XkbSA_LockMods);

                        // maybe use action's mask
                        if ((action->mods.flags & XkbSA_UseModMapMods) == 0) {
                            modifierMask = action->mods.mask;
                        }
                    }
                    else if (action->type == XkbSA_SetGroup ||
                            action->type == XkbSA_LatchGroup ||
                            action->type == XkbSA_LockGroup) {
                        // ignore group change key
                        continue;
                    }
                }
                level = mapEntry->level;

                // VMware modifier hack
                if (useLastGoodModifiers) {
                    XKBModifierMap::const_iterator k =
                        m_lastGoodXKBModifiers.find(eGroup * 256 + keycode);
                    if (k != m_lastGoodXKBModifiers.end()) {
                        // Use last known good modifier
                        isModifier   = true;
                        level        = k->second.m_level;
                        modifierMask = k->second.m_mask;
                        item.m_lock  = k->second.m_lock;
                    }
                }
                else if (isModifier) {
                    // Save known good modifier
                    XKBModifierInfo& info =
                        m_lastGoodXKBModifiers[eGroup * 256 + keycode];
                    info.m_level = level;
                    info.m_mask  = modifierMask;
                    info.m_lock  = item.m_lock;
                }

                // record the modifier mask for this key.  don't bother
                // for keys that change the group.
                item.m_generates = 0;
                UInt32 modifierBit =
                    XWindowsUtil::getModifierBitForKeySym(keysym);
                if (isModifier && modifierBit != kKeyModifierBitNone) {
                    item.m_generates = (1u << modifierBit);
                    for (SInt32 j = 0; j < 8; ++j) {
                        // skip modifiers this key doesn't generate
                        if ((modifierMask & (1u << j)) == 0) {
                            continue;
                        }

                        // skip keys that map to a modifier that we've
                        // already seen using fewer modifiers.  that is
                        // if this key must combine with other modifiers
                        // and we know of a key that combines with fewer
                        // modifiers (or no modifiers) then prefer the
                        // other key.
                        if (level >= modifierLevel[8 * group + j]) {
                            continue;
                        }
                        modifierLevel[8 * group + j] = level;

                        // save modifier
                        m_modifierFromX[8 * group + j] |= (1u << modifierBit);
                        m_modifierToX.insert(std::make_pair(
                                1u << modifierBit, 1u << j));
                    }
                }

                // handle special cases of just one keysym for the keycode
                if (type->num_levels == 1) {
                    // if there are upper- and lowercase versions of the
                    // keysym then add both.
                    KeySym lKeysym, uKeysym;
                    XConvertCase(keysym, &lKeysym, &uKeysym);
                    if (lKeysym != uKeysym) {
                        if (j != -1) {
                            continue;
                        }

                        item.m_sensitive |= ShiftMask | LockMask;

                        KeyID lKeyID = XWindowsUtil::mapKeySymToKeyID(lKeysym);
                        KeyID uKeyID = XWindowsUtil::mapKeySymToKeyID(uKeysym);
                        if (lKeyID == kKeyNone || uKeyID == kKeyNone) {
                            continue;
                        }

                        item.m_id       = lKeyID;
                        item.m_required = 0;
                        keyMap.addKeyEntry(item);

                        item.m_id       = uKeyID;
                        item.m_required = ShiftMask;
                        keyMap.addKeyEntry(item);
                        item.m_required = LockMask;
                        keyMap.addKeyEntry(item);

                        if (group == 0) {
                            m_keyCodeFromKey.insert(
                                    std::make_pair(lKeyID, keycode));
                            m_keyCodeFromKey.insert(
                                    std::make_pair(uKeyID, keycode));
                        }
                        continue;
                    }
                }

                // add entry
                item.m_id = XWindowsUtil::mapKeySymToKeyID(keysym);
                keyMap.addKeyEntry(item);
                if (group == 0) {
                    m_keyCodeFromKey.insert(std::make_pair(item.m_id, keycode));
                }
            }
        }
    }

    // change all modifier masks to barrier masks from X masks
    keyMap.foreachKey(&XWindowsKeyState::remapKeyModifiers, this);

    // allow composition across groups
    keyMap.allowGroupSwitchDuringCompose();
}
#endif

void
XWindowsKeyState::remapKeyModifiers(KeyID id, SInt32 group,
                            barrier::KeyMap::KeyItem& item, void* vself)
{
    XWindowsKeyState* self = static_cast<XWindowsKeyState*>(vself);
    item.m_required  =
        self->mapModifiersFromX(XkbBuildCoreState(item.m_required, group));
    item.m_sensitive =
        self->mapModifiersFromX(XkbBuildCoreState(item.m_sensitive, group));
}

bool
XWindowsKeyState::hasModifiersXKB() const
{
#if HAVE_XKB_EXTENSION
    // iterate over all keycodes
    for (int i = m_xkb->min_key_code; i <= m_xkb->max_key_code; ++i) {
        KeyCode keycode = static_cast<KeyCode>(i);
        if (m_impl->do_XkbKeyHasActions(m_xkb, keycode) == True) {
            // iterate over all groups
            int numGroups = m_impl->do_XkbKeyNumGroups(m_xkb, keycode);
            for (int group = 0; group < numGroups; ++group) {
                // iterate over all shift levels for the button (including none)
                XkbKeyTypePtr type = m_impl->do_XkbKeyKeyType(m_xkb, keycode, group);
                for (int j = -1; j < type->map_count; ++j) {
                    if (j != -1 && !type->map[j].active) {
                        continue;
                    }
                    int level = ((j == -1) ? 0 : type->map[j].level);
                    XkbAction* action =
                        m_impl->do_XkbKeyActionEntry(m_xkb, keycode, level, group);
                    if (action->type == XkbSA_SetMods ||
                        action->type == XkbSA_LockMods) {
                        return true;
                    }
                }
            }
        }
    }
#endif
    return false;
}

int
XWindowsKeyState::getEffectiveGroup(KeyCode keycode, int group) const
{
    (void)keycode;
#if HAVE_XKB_EXTENSION
    // get effective group for key
    int numGroups = m_impl->do_XkbKeyNumGroups(m_xkb, keycode);
    if (group >= numGroups) {
        unsigned char groupInfo = m_impl->do_XkbKeyGroupInfo(m_xkb, keycode);
        switch (XkbOutOfRangeGroupAction(groupInfo)) {
        case XkbClampIntoRange:
            group = numGroups - 1;
            break;

        case XkbRedirectIntoRange:
            group = XkbOutOfRangeGroupNumber(groupInfo);
            if (group >= numGroups) {
                group = 0;
            }
            break;

        default:
            // wrap
            group %= numGroups;
            break;
        }
    }
#endif
    return group;
}

UInt32
XWindowsKeyState::getGroupFromState(unsigned int state) const
{
#if HAVE_XKB_EXTENSION
    if (m_xkb != NULL) {
        return XkbGroupForCoreState(state);
    }
#endif
    return 0;
}
