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

#pragma once

#include "synergy/KeyState.h"
#include "platform/IOSXKeyResource.h"

#include <Carbon/Carbon.h>

typedef TISInputSourceRef KeyLayout;

class OSXUchrKeyResource : public IOSXKeyResource {
public:
    OSXUchrKeyResource (const void*, UInt32 keyboardType);

    // KeyResource overrides
    virtual bool isValid () const;
    virtual UInt32 getNumModifierCombinations () const;
    virtual UInt32 getNumTables () const;
    virtual UInt32 getNumButtons () const;
    virtual UInt32 getTableForModifier (UInt32 mask) const;
    virtual KeyID getKey (UInt32 table, UInt32 button) const;

private:
    typedef std::vector<KeyID> KeySequence;

    bool getDeadKey (KeySequence& keys, UInt16 index) const;
    bool getKeyRecord (KeySequence& keys, UInt16 index, UInt16& state) const;
    bool addSequence (KeySequence& keys, UCKeyCharSeq c) const;

private:
    const UCKeyboardLayout* m_resource;
    const UCKeyModifiersToTableNum* m_m;
    const UCKeyToCharTableIndex* m_cti;
    const UCKeySequenceDataIndex* m_sdi;
    const UCKeyStateRecordsIndex* m_sri;
    const UCKeyStateTerminators* m_st;
    UInt16 m_spaceOutput;
};
