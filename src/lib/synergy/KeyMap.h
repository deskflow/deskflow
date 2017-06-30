/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2005 Chris Schoeneman
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
#include "base/String.h"
#include "common/stdmap.h"
#include "common/stdset.h"
#include "common/stdvector.h"

#include <gtest/gtest_prod.h>

namespace synergy {

//! Key map
/*!
This class provides a keyboard mapping.
*/
class KeyMap {
public:
    KeyMap ();
    virtual ~KeyMap ();

    //! KeyID synthesis info
    /*!
    This structure contains the information necessary to synthesize a
    keystroke that generates a KeyID (stored elsewhere).  \c m_sensitive
    lists the modifiers that the key is affected by and must therefore
    be in the correct state, which is listed in \c m_required.  If the
    key is mapped to a modifier, that modifier is in \c m_generates and
    is not in \c m_sensitive.
    */
    struct KeyItem {
    public:
        KeyID m_id;                  //!< KeyID
        SInt32 m_group;              //!< Group for key
        KeyButton m_button;          //!< Button to generate KeyID
        KeyModifierMask m_required;  //!< Modifiers required for KeyID
        KeyModifierMask m_sensitive; //!< Modifiers key is sensitive to
        KeyModifierMask m_generates; //!< Modifiers key is mapped to
        bool m_dead;                 //!< \c true if this is a dead KeyID
        bool m_lock;                 //!< \c true if this locks a modifier
        UInt32 m_client;             //!< Client data

    public:
        bool operator== (const KeyItem&) const;
    };

    //! The KeyButtons needed to synthesize a KeyID
    /*!
    An ordered list of \c KeyItems produces a particular KeyID.  If
    the KeyID can be synthesized directly then there is one entry in
    the list.  If dead keys are required then they're listed first.
    A list is the minimal set of keystrokes necessary to synthesize
    the KeyID, so it doesn't include no-ops.  A list does not include
    any modifier keys unless the KeyID is a modifier, in which case
    it has exactly one KeyItem for the modifier itself.
    */
    typedef std::vector<KeyItem> KeyItemList;

    //! A keystroke
    class Keystroke {
    public:
        enum EType {
            kButton, //!< Synthesize button
            kGroup   //!< Set new group
        };

        Keystroke (KeyButton, bool press, bool repeat, UInt32 clientData);
        Keystroke (SInt32 group, bool absolute, bool restore);

    public:
        struct Button {
        public:
            KeyButton m_button; //!< Button to synthesize
            bool m_press;       //!< \c true iff press
            bool m_repeat;      //!< \c true iff for an autorepeat
            UInt32 m_client;    //!< Client data
        };
        struct Group {
        public:
            SInt32 m_group;  //!< Group/offset to change to/by
            bool m_absolute; //!< \c true iff change to, else by
            bool m_restore;  //!< \c true iff for restoring state
        };
        union Data {
        public:
            Button m_button;
            Group m_group;
        };

        EType m_type;
        Data m_data;
    };

    //! A sequence of keystrokes
    typedef std::vector<Keystroke> Keystrokes;

    //! A mapping of a modifier to keys for that modifier
    typedef std::multimap<KeyModifierMask, KeyItem> ModifierToKeys;

    //! A set of buttons
    typedef std::map<KeyButton, const KeyItem*> ButtonToKeyMap;

    //! Callback type for \c foreachKey
    typedef void (*ForeachKeyCallback) (KeyID, SInt32 group, KeyItem&,
                                        void* userData);

    //! @name manipulators
    //@{

    //! Swap with another \c KeyMap
    virtual void swap (KeyMap&);

    //! Add a key entry
    /*!
    Adds \p item to the entries for the item's id and group.  The
    \c m_dead member is set automatically.
    */
    void addKeyEntry (const KeyItem& item);

    //! Add an alias key entry
    /*!
    If \p targetID with the modifiers given by \p targetRequired and
    \p targetSensitive is not available in group \p group then find an
    entry for \p sourceID with modifiers given by \p sourceRequired and
    \p sourceSensitive in any group with exactly one item and, if found,
    add a new item just like it except using id \p targetID.  This
    effectively makes the \p sourceID an alias for \p targetID (i.e. we
    can generate \p targetID using \p sourceID).
    */
    void addKeyAliasEntry (KeyID targetID, SInt32 group,
                           KeyModifierMask targetRequired,
                           KeyModifierMask targetSensitive, KeyID sourceID,
                           KeyModifierMask sourceRequired,
                           KeyModifierMask sourceSensitive);

    //! Add a key sequence entry
    /*!
    Adds the sequence of keys \p keys (\p numKeys elements long) to
    synthesize key \p id in group \p group.  This looks up in the
    map each key in \p keys.  If all are found then each key is
    converted to the button for that key and the buttons are added
    as the entry for \p id.  If \p id is already in the map or at
    least one key in \p keys is not in the map then nothing is added
    and this returns \c false, otherwise it returns \c true.
    */
    bool addKeyCombinationEntry (KeyID id, SInt32 group, const KeyID* keys,
                                 UInt32 numKeys);

    //! Enable composition across groups
    /*!
    If called then the keyboard map will allow switching between groups
    during key composition.  Not all systems allow that.
    */
    void allowGroupSwitchDuringCompose ();

    //! Add a half-duplex button
    /*!
    Records that button \p button is a half-duplex key.  This is called
    when translating the system's keyboard map.  It's independent of the
    half-duplex modifier calls.
    */
    void addHalfDuplexButton (KeyButton button);

    //! Remove all half-duplex modifiers
    /*!
    Removes all half-duplex modifiers.  This is called to set user
    configurable half-duplex settings.
    */
    void clearHalfDuplexModifiers ();

    //! Add a half-duplex modifier
    /*!
    Records that modifier key \p key is half-duplex.  This is called to
    set user configurable half-duplex settings.
    */
    virtual void addHalfDuplexModifier (KeyID key);

    //! Finish adding entries
    /*!
    Called after adding entries, this does some internal housekeeping.
    */
    virtual void finish ();

    //! Iterate over all added keys items
    /*!
    Calls \p cb for every key item.
    */
    virtual void foreachKey (ForeachKeyCallback cb, void* userData);

    //@}
    //! @name accessors
    //@{

    //! Map key press/repeat to keystrokes.
    /*!
    Converts press/repeat of key \p id in group \p group with current
    modifiers as given in \p currentState and the desired modifiers in
    \p desiredMask into the keystrokes necessary to synthesize that key
    event in \p keys.  It returns the \c KeyItem of the key being
    pressed/repeated, or NULL if the key cannot be mapped.
    */
    virtual const KeyItem*
    mapKey (Keystrokes& keys, KeyID id, SInt32 group,
            ModifierToKeys& activeModifiers, KeyModifierMask& currentState,
            KeyModifierMask desiredMask, bool isAutoRepeat) const;

    //! Get number of groups
    /*!
    Returns the number of keyboard groups (independent layouts) in the map.
    */
    SInt32 getNumGroups () const;

    //! Compute a group number
    /*!
    Returns the number of the group \p offset groups after group \p group.
    */
    SInt32 getEffectiveGroup (SInt32 group, SInt32 offset) const;

    //! Find key entry compatible with modifiers
    /*!
    Returns the \c KeyItemList for the first entry for \p id in group
    \p group that is compatible with the given modifiers, or NULL
    if there isn't one.  A button list is compatible with a modifiers
    if it is either insensitive to all modifiers in \p sensitive or
    it requires the modifiers to be in the state indicated by \p required
    for every modifier indicated by \p sensitive.
    */
    const KeyItemList*
    findCompatibleKey (KeyID id, SInt32 group, KeyModifierMask required,
                       KeyModifierMask sensitive) const;

    //! Test if modifier is half-duplex
    /*!
    Returns \c true iff modifier key \p key or button \p button is
    half-duplex.
    */
    virtual bool isHalfDuplex (KeyID key, KeyButton button) const;

    //! Test if modifiers indicate a command
    /*!
    Returns \c true iff the modifiers in \p mask contain any command
    modifiers.  A command modifier is used for keyboard shortcuts and
    hotkeys,  Rather than trying to synthesize a character, a command
    is trying to synthesize a particular set of buttons.  So it's not
    important to match the shift or AltGr state to achieve a character
    but it is important to match the modifier state exactly.
    */
    bool isCommand (KeyModifierMask mask) const;

    // Get the modifiers that indicate a command
    /*!
    Returns the modifiers that when combined with other keys indicate
    a command (e.g. shortcut or hotkey).
    */
    KeyModifierMask getCommandModifiers () const;

    //! Get buttons from modifier map
    /*!
    Put all the keys in \p modifiers into \p keys.
    */
    static void
    collectButtons (const ModifierToKeys& modifiers, ButtonToKeyMap& keys);

    //! Set modifier key state
    /*!
    Sets the modifier key state (\c m_generates and \c m_lock) in \p item
    based on the \c m_id in \p item.
    */
    static void initModifierKey (KeyItem& item);

    //! Test for a dead key
    /*!
    Returns \c true if \p key is a dead key.
    */
    static bool isDeadKey (KeyID key);

    //! Get corresponding dead key
    /*!
    Returns the dead key corresponding to \p key if one exists, otherwise
    return \c kKeyNone.  This returns \p key if it's already a dead key.
    */
    static KeyID getDeadKey (KeyID key);

    //! Get string for a key and modifier mask
    /*!
    Converts a key and modifier mask into a string representing the
    combination.
    */
    static String formatKey (KeyID key, KeyModifierMask);

    //! Parse a string into a key
    /*!
    Converts a string into a key.  Returns \c true on success and \c false
    if the string cannot be parsed.
    */
    static bool parseKey (const String&, KeyID&);

    //! Parse a string into a modifier mask
    /*!
    Converts a string into a modifier mask.  Returns \c true on success
    and \c false if the string cannot be parsed.  The modifiers plus any
    remaining leading and trailing whitespace is stripped from the input
    string.
    */
    static bool parseModifiers (String&, KeyModifierMask&);

    //@}

private:
    FRIEND_TEST (KeyMapTests, findBestKey_requiredDown_matchExactFirstItem);
    FRIEND_TEST (KeyMapTests,
                 findBestKey_requiredAndExtraSensitiveDown_matchExactFirstItem);
    FRIEND_TEST (
        KeyMapTests,
        findBestKey_requiredAndExtraSensitiveDown_matchExactSecondItem);
    FRIEND_TEST (KeyMapTests,
                 findBestKey_extraSensitiveDown_matchExactSecondItem);
    FRIEND_TEST (KeyMapTests,
                 findBestKey_noRequiredDown_matchOneRequiredChangeItem);
    FRIEND_TEST (KeyMapTests,
                 findBestKey_onlyOneRequiredDown_matchTwoRequiredChangesItem);
    FRIEND_TEST (KeyMapTests, findBestKey_noRequiredDown_cannotMatch);

private:
    //! Ways to synthesize a key
    enum EKeystroke {
        kKeystrokePress,   //!< Synthesize a press
        kKeystrokeRelease, //!< Synthesize a release
        kKeystrokeRepeat,  //!< Synthesize an autorepeat
        kKeystrokeClick,   //!< Synthesize a press and release
        kKeystrokeModify,  //!< Synthesize pressing a modifier
        kKeystrokeUnmodify //!< Synthesize releasing a modifier
    };

    // A list of ways to synthesize a KeyID
    typedef std::vector<KeyItemList> KeyEntryList;

    // computes the number of groups
    SInt32 findNumGroups () const;

    // computes the map of modifiers to the keys that generate the modifiers
    void setModifierKeys ();

    // maps a command key.  a command key is a keyboard shortcut and we're
    // trying to synthesize a button press with an exact sets of modifiers,
    // not trying to synthesize a character.  so we just need to find the
    // right button and synthesize the requested modifiers without regard
    // to what character they would synthesize.  we disallow multikey
    // entries since they don't make sense as hotkeys.
    const KeyItem*
    mapCommandKey (Keystrokes& keys, KeyID id, SInt32 group,
                   ModifierToKeys& activeModifiers,
                   KeyModifierMask& currentState, KeyModifierMask desiredMask,
                   bool isAutoRepeat) const;

    // maps a character key.  a character key is trying to synthesize a
    // particular KeyID and isn't entirely concerned with the modifiers
    // used to do it.
    const KeyItem*
    mapCharacterKey (Keystrokes& keys, KeyID id, SInt32 group,
                     ModifierToKeys& activeModifiers,
                     KeyModifierMask& currentState, KeyModifierMask desiredMask,
                     bool isAutoRepeat) const;

    // maps a modifier key
    const KeyItem*
    mapModifierKey (Keystrokes& keys, KeyID id, SInt32 group,
                    ModifierToKeys& activeModifiers,
                    KeyModifierMask& currentState, KeyModifierMask desiredMask,
                    bool isAutoRepeat) const;

    // returns the index into \p entryList of the KeyItemList requiring
    // the fewest modifier changes between \p currentState and
    // \p desiredState.
    SInt32
    findBestKey (const KeyEntryList& entryList, KeyModifierMask currentState,
                 KeyModifierMask desiredState) const;

    // gets the \c KeyItem used to synthesize the modifier who's bit is
    // given by \p modifierBit in group \p group and does not synthesize
    // the key \p button.
    const KeyItem*
    keyForModifier (KeyButton button, SInt32 group, SInt32 modifierBit) const;

    // fills \p keystrokes with the keys to synthesize the key in
    // \p keyItem taking the modifiers into account.  returns \c true
    // iff successful and sets \p currentState to the
    // resulting modifier state.
    bool
    keysForKeyItem (const KeyItem& keyItem, SInt32& group,
                    ModifierToKeys& activeModifiers,
                    KeyModifierMask& currentState, KeyModifierMask desiredState,
                    KeyModifierMask overrideModifiers, bool isAutoRepeat,
                    Keystrokes& keystrokes) const;

    // fills \p keystrokes with the keys to synthesize the modifiers
    // in \p desiredModifiers from the active modifiers listed in
    // \p activeModifiers not including the key in \p keyItem.
    // returns \c true iff successful.
    bool keysToRestoreModifiers (const KeyItem& keyItem, SInt32 group,
                                 ModifierToKeys& activeModifiers,
                                 KeyModifierMask& currentState,
                                 const ModifierToKeys& desiredModifiers,
                                 Keystrokes& keystrokes) const;

    // fills \p keystrokes and \p undo with the keys to change the
    // current modifier state in \p currentState to match the state in
    // \p requiredState for each modifier indicated in \p sensitiveMask.
    // returns \c true iff successful and sets \p currentState to the
    // resulting modifier state.
    bool keysForModifierState (KeyButton button, SInt32 group,
                               ModifierToKeys& activeModifiers,
                               KeyModifierMask& currentState,
                               KeyModifierMask requiredState,
                               KeyModifierMask sensitiveMask,
                               KeyModifierMask notRequiredMask,
                               Keystrokes& keystrokes) const;

    // Adds keystrokes to synthesize key \p keyItem in mode \p type to
    // \p keystrokes and to undo the synthesis to \p undo.
    void
    addKeystrokes (EKeystroke type, const KeyItem& keyItem,
                   ModifierToKeys& activeModifiers,
                   KeyModifierMask& currentState, Keystrokes& keystrokes) const;

    // Returns the number of modifiers indicated in \p state.
    static SInt32 getNumModifiers (KeyModifierMask state);

    // Initialize key name/id maps
    static void initKeyNameMaps ();

    // not implemented
    KeyMap (const KeyMap&);
    KeyMap& operator= (const KeyMap&);

private:
    // Ways to synthesize a KeyID over multiple keyboard groups
    typedef std::vector<KeyEntryList> KeyGroupTable;

    // Table of KeyID to ways to synthesize that KeyID
    typedef std::map<KeyID, KeyGroupTable> KeyIDMap;

    // List of KeyItems that generate a particular modifier
    typedef std::vector<const KeyItem*> ModifierKeyItemList;

    // Map a modifier to the KeyItems that synthesize that modifier
    typedef std::vector<ModifierKeyItemList> ModifierToKeyTable;

    // A set of keys
    typedef std::set<KeyID> KeySet;

    // A set of buttons
    typedef std::set<KeyButton> KeyButtonSet;

    // Key maps for parsing/formatting
    typedef std::map<String, KeyID, synergy::string::CaselessCmp> NameToKeyMap;
    typedef std::map<String, KeyModifierMask, synergy::string::CaselessCmp>
        NameToModifierMap;
    typedef std::map<KeyID, String> KeyToNameMap;
    typedef std::map<KeyModifierMask, String> ModifierToNameMap;

    // KeyID info
    KeyIDMap m_keyIDMap;
    SInt32 m_numGroups;
    ModifierToKeyTable m_modifierKeys;

    // composition info
    bool m_composeAcrossGroups;

    // half-duplex info
    KeyButtonSet m_halfDuplex; // half-duplex set by synergy
    KeySet m_halfDuplexMods;   // half-duplex set by user

    // dummy KeyItem for changing modifiers
    KeyItem m_modifierKeyItem;

    // parsing/formatting tables
    static NameToKeyMap* s_nameToKeyMap;
    static NameToModifierMap* s_nameToModifierMap;
    static KeyToNameMap* s_keyToNameMap;
    static ModifierToNameMap* s_modifierToNameMap;
};
}
