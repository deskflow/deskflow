/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IKeyState.h"
#include "deskflow/KeyMap.h"

//! Core key state
/*!
This class provides key state services.  Subclasses must implement a few
platform specific methods.
*/
class KeyState : public IKeyState
{
public:
  KeyState(IEventQueue *events, std::vector<std::string> layouts, bool isLangSyncEnabled);
  KeyState(IEventQueue *events, deskflow::KeyMap &keyMap, std::vector<std::string> layouts, bool isLangSyncEnabled);
  virtual ~KeyState();

  //! @name manipulators
  //@{

  //! Handle key event
  /*!
  Sets the state of \p button to down or up and updates the current
  modifier state to \p newState.  This method should be called by
  primary screens only in response to local events.  For auto-repeat
  set \p down to \c true.  Overrides must forward to the superclass.
  */
  virtual void onKey(KeyButton button, bool down, KeyModifierMask newState);

  //! Post a key event
  /*!
  Posts a key event.  This may adjust the event or post additional
  events in some circumstances.  If this is overridden it must forward
  to the superclass.
  */
  virtual void sendKeyEvent(
      void *target, bool press, bool isAutoRepeat, KeyID key, KeyModifierMask mask, int32_t count, KeyButton button
  );

  //@}
  //! @name accessors
  //@{

  //@}

  void updateKeyMap(deskflow::KeyMap *existing);
  // IKeyState overrides
  void updateKeyMap() override
  {
    this->updateKeyMap(nullptr);
  }
  void updateKeyState() override;
  void setHalfDuplexMask(KeyModifierMask) override;
  void fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang) override;
  bool fakeKeyRepeat(KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang) override;
  bool fakeKeyUp(KeyButton button) override;
  void fakeAllKeysUp() override;
  bool fakeMediaKey(KeyID id) override;

  bool isKeyDown(KeyButton) const override;
  KeyModifierMask getActiveModifiers() const override;
  // Left abstract
  virtual bool fakeCtrlAltDel() override = 0;
  virtual KeyModifierMask pollActiveModifiers() const override = 0;
  virtual int32_t pollActiveGroup() const override = 0;
  virtual void pollPressedKeys(KeyButtonSet &pressedKeys) const override = 0;

  int32_t getKeyState(KeyButton keyButton)
  {
    return m_keys[keyButton];
  }

protected:
  using Keystroke = deskflow::KeyMap::Keystroke;

  //! @name protected manipulators
  //@{

  //! Get the keyboard map
  /*!
  Fills \p keyMap with the current keyboard map.
  */
  virtual void getKeyMap(deskflow::KeyMap &keyMap) = 0;

  //! Fake a key event
  /*!
  Synthesize an event for \p keystroke.
  */
  virtual void fakeKey(const Keystroke &keystroke) = 0;

  //! Get the active modifiers
  /*!
  Returns the modifiers that are currently active according to our
  shadowed state.  The state may be modified.
  */
  virtual KeyModifierMask &getActiveModifiersRValue();

  //@}
  //! @name protected accessors
  //@{

  //! Compute a group number
  /*!
  Returns the number of the group \p offset groups after group \p group.
  */
  int32_t getEffectiveGroup(int32_t group, int32_t offset) const;

  //! Check if key is ignored
  /*!
  Returns \c true if and only if the key should always be ignored.
  The default returns \c true only for the toggle keys.
  */
  virtual bool isIgnoredKey(KeyID key, KeyModifierMask mask) const;

  //! Get button for a KeyID
  /*!
  Return the button mapped to key \p id in group \p group if any,
  otherwise returns 0.
  */
  KeyButton getButton(KeyID id, int32_t group) const;

  //@}

private:
  using Keystrokes = deskflow::KeyMap::Keystrokes;
  using ModifierToKeys = deskflow::KeyMap::ModifierToKeys;

public:
  struct AddActiveModifierContext
  {
  public:
    AddActiveModifierContext(int32_t group, KeyModifierMask mask, ModifierToKeys &activeModifiers);

  public:
    int32_t m_activeGroup;
    KeyModifierMask m_mask;
    ModifierToKeys &m_activeModifiers;

  private:
    // not implemented
    AddActiveModifierContext(const AddActiveModifierContext &);
    AddActiveModifierContext &operator=(const AddActiveModifierContext &);
  };

private:
  class ButtonToKeyLess
  {
  public:
    bool operator()(
        const deskflow::KeyMap::ButtonToKeyMap::value_type &a, const deskflow::KeyMap::ButtonToKeyMap::value_type b
    ) const
    {
      return (a.first < b.first);
    }
  };

  // not implemented
  KeyState(const KeyState &);
  KeyState &operator=(const KeyState &);

  // called by all ctors.
  void init();

  // adds alias key sequences.  these are sequences that are equivalent
  // to other sequences.
  void addAliasEntries();

  // adds non-keypad key sequences for keypad KeyIDs
  void addKeypadEntries();

  // adds key sequences for combination KeyIDs (those built using
  // dead keys)
  void addCombinationEntries();

  // synthesize key events.  synthesize auto-repeat events count times.
  void fakeKeys(const Keystrokes &, uint32_t count);

  // update key state to match changes to modifiers
  void updateModifierKeyState(KeyButton button, const ModifierToKeys &oldModifiers, const ModifierToKeys &newModifiers);

  // active modifiers collection callback
  static void addActiveModifierCB(KeyID id, int32_t group, deskflow::KeyMap::KeyItem &keyItem, void *vcontext);

private:
  // must be declared before m_keyMap. used when this class owns the key map.
  deskflow::KeyMap *m_keyMapPtr;

  // the keyboard map
  deskflow::KeyMap &m_keyMap;

  // current modifier state
  KeyModifierMask m_mask;

  // the active modifiers and the buttons activating them
  ModifierToKeys m_activeModifiers;

  // current keyboard state (> 0 if pressed, 0 otherwise).  this is
  // initialized to the keyboard state according to the system then
  // it tracks synthesized events.
  int32_t m_keys[kNumButtons];

  // synthetic keyboard state (> 0 if pressed, 0 otherwise).  this
  // tracks the synthesized keyboard state.  if m_keys[n] > 0 but
  // m_syntheticKeys[n] == 0 then the key was pressed locally and
  // not synthesized yet.
  int32_t m_syntheticKeys[kNumButtons];

  // client data for each pressed key
  uint32_t m_keyClientData[kNumButtons];

  // server keyboard state.  an entry is 0 if not the key isn't pressed
  // otherwise it's the local KeyButton synthesized for the server key.
  KeyButton m_serverKeys[kNumButtons];

  IEventQueue *m_events;

  bool m_isLangSyncEnabled;
};
