/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless Ltd.
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

#include "platform/OSXUchrKeyResource.h"

#include <Carbon/Carbon.h>

//
// OSXUchrKeyResource
//

OSXUchrKeyResource::OSXUchrKeyResource (const void* resource,
                                        UInt32 keyboardType)
    : m_m (NULL), m_cti (NULL), m_sdi (NULL), m_sri (NULL), m_st (NULL) {
    m_resource = static_cast<const UCKeyboardLayout*> (resource);
    if (m_resource == NULL) {
        return;
    }

    // find the keyboard info for the current keyboard type
    const UCKeyboardTypeHeader* th = NULL;
    const UCKeyboardLayout* r      = m_resource;
    for (ItemCount i = 0; i < r->keyboardTypeCount; ++i) {
        if (keyboardType >= r->keyboardTypeList[i].keyboardTypeFirst &&
            keyboardType <= r->keyboardTypeList[i].keyboardTypeLast) {
            th = r->keyboardTypeList + i;
            break;
        }
        if (r->keyboardTypeList[i].keyboardTypeFirst == 0) {
            // found the default.  use it unless we find a match.
            th = r->keyboardTypeList + i;
        }
    }
    if (th == NULL) {
        // cannot find a suitable keyboard type
        return;
    }

    // get tables for keyboard type
    const UInt8* const base = reinterpret_cast<const UInt8*> (m_resource);
    m_m = reinterpret_cast<const UCKeyModifiersToTableNum*> (
        base + th->keyModifiersToTableNumOffset);
    m_cti = reinterpret_cast<const UCKeyToCharTableIndex*> (
        base + th->keyToCharTableIndexOffset);
    m_sdi = reinterpret_cast<const UCKeySequenceDataIndex*> (
        base + th->keySequenceDataIndexOffset);
    if (th->keyStateRecordsIndexOffset != 0) {
        m_sri = reinterpret_cast<const UCKeyStateRecordsIndex*> (
            base + th->keyStateRecordsIndexOffset);
    }
    if (th->keyStateTerminatorsOffset != 0) {
        m_st = reinterpret_cast<const UCKeyStateTerminators*> (
            base + th->keyStateTerminatorsOffset);
    }

    // find the space key, but only if it can combine with dead keys.
    // a dead key followed by a space yields the non-dead version of
    // the dead key.
    m_spaceOutput = 0xffffu;
    UInt32 table  = getTableForModifier (0);
    for (UInt32 button = 0, n = getNumButtons (); button < n; ++button) {
        KeyID id = getKey (table, button);
        if (id == 0x20) {
            UCKeyOutput c = reinterpret_cast<const UCKeyOutput*> (
                base + m_cti->keyToCharTableOffsets[table])[button];
            if ((c & kUCKeyOutputTestForIndexMask) ==
                kUCKeyOutputStateIndexMask) {
                m_spaceOutput = (c & kUCKeyOutputGetIndexMask);
                break;
            }
        }
    }
}

bool
OSXUchrKeyResource::isValid () const {
    return (m_m != NULL);
}

UInt32
OSXUchrKeyResource::getNumModifierCombinations () const {
    // only 32 (not 256) because the righthanded modifier bits are ignored
    return 32;
}

UInt32
OSXUchrKeyResource::getNumTables () const {
    return m_cti->keyToCharTableCount;
}

UInt32
OSXUchrKeyResource::getNumButtons () const {
    return m_cti->keyToCharTableSize;
}

UInt32
OSXUchrKeyResource::getTableForModifier (UInt32 mask) const {
    if (mask >= m_m->modifiersCount) {
        return m_m->defaultTableNum;
    } else {
        return m_m->tableNum[mask];
    }
}

KeyID
OSXUchrKeyResource::getKey (UInt32 table, UInt32 button) const {
    assert (table < getNumTables ());
    assert (button < getNumButtons ());

    const UInt8* const base = reinterpret_cast<const UInt8*> (m_resource);
    const UCKeyOutput* cPtr = reinterpret_cast<const UCKeyOutput*> (
        base + m_cti->keyToCharTableOffsets[table]);

    const UCKeyOutput c = cPtr[button];

    KeySequence keys;
    switch (c & kUCKeyOutputTestForIndexMask) {
        case kUCKeyOutputStateIndexMask:
            if (!getDeadKey (keys, c & kUCKeyOutputGetIndexMask)) {
                return kKeyNone;
            }
            break;

        case kUCKeyOutputSequenceIndexMask:
        default:
            if (!addSequence (keys, c)) {
                return kKeyNone;
            }
            break;
    }

    // XXX -- no support for multiple characters
    if (keys.size () != 1) {
        return kKeyNone;
    }

    return keys.front ();
}

bool
OSXUchrKeyResource::getDeadKey (KeySequence& keys, UInt16 index) const {
    if (m_sri == NULL || index >= m_sri->keyStateRecordCount) {
        // XXX -- should we be using some other fallback?
        return false;
    }

    UInt16 state = 0;
    if (!getKeyRecord (keys, index, state)) {
        return false;
    }
    if (state == 0) {
        // not a dead key
        return true;
    }

    // no dead keys if we couldn't find the space key
    if (m_spaceOutput == 0xffffu) {
        return false;
    }

    // the dead key should not have put anything in the key list
    if (!keys.empty ()) {
        return false;
    }

    // get the character generated by pressing the space key after the
    // dead key.  if we're still in a compose state afterwards then we're
    // confused so we bail.
    if (!getKeyRecord (keys, m_spaceOutput, state) || state != 0) {
        return false;
    }

    // convert keys to their dead counterparts
    for (KeySequence::iterator i = keys.begin (); i != keys.end (); ++i) {
        *i = synergy::KeyMap::getDeadKey (*i);
    }

    return true;
}

bool
OSXUchrKeyResource::getKeyRecord (KeySequence& keys, UInt16 index,
                                  UInt16& state) const {
    const UInt8* const base    = reinterpret_cast<const UInt8*> (m_resource);
    const UCKeyStateRecord* sr = reinterpret_cast<const UCKeyStateRecord*> (
        base + m_sri->keyStateRecordOffsets[index]);
    const UCKeyStateEntryTerminal* kset =
        reinterpret_cast<const UCKeyStateEntryTerminal*> (sr->stateEntryData);

    UInt16 nextState = 0;
    bool found       = false;
    if (state == 0) {
        found     = true;
        nextState = sr->stateZeroNextState;
        if (!addSequence (keys, sr->stateZeroCharData)) {
            return false;
        }
    } else {
        // we have a next entry
        switch (sr->stateEntryFormat) {
            case kUCKeyStateEntryTerminalFormat:
                for (UInt16 j = 0; j < sr->stateEntryCount; ++j) {
                    if (kset[j].curState == state) {
                        if (!addSequence (keys, kset[j].charData)) {
                            return false;
                        }
                        nextState = 0;
                        found     = true;
                        break;
                    }
                }
                break;

            case kUCKeyStateEntryRangeFormat:
                // XXX -- not supported yet
                break;

            default:
                // XXX -- unknown format
                return false;
        }
    }
    if (!found) {
        // use a terminator
        if (m_st != NULL && state < m_st->keyStateTerminatorCount) {
            if (!addSequence (keys, m_st->keyStateTerminators[state - 1])) {
                return false;
            }
        }
        nextState = sr->stateZeroNextState;
        if (!addSequence (keys, sr->stateZeroCharData)) {
            return false;
        }
    }

    // next
    state = nextState;

    return true;
}

bool
OSXUchrKeyResource::addSequence (KeySequence& keys, UCKeyCharSeq c) const {
    if ((c & kUCKeyOutputTestForIndexMask) == kUCKeyOutputSequenceIndexMask) {
        UInt16 index = (c & kUCKeyOutputGetIndexMask);
        if (index < m_sdi->charSequenceCount &&
            m_sdi->charSequenceOffsets[index] !=
                m_sdi->charSequenceOffsets[index + 1]) {
            // XXX -- sequences not supported yet
            return false;
        }
    }

    if (c != 0xfffe && c != 0xffff) {
        KeyID id = unicharToKeyID (c);
        if (id != kKeyNone) {
            keys.push_back (id);
        }
    }

    return true;
}
