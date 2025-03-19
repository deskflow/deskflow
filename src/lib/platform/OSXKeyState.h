/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "OSXAutoTypes.h"
#include "common/StdMap.h"
#include "common/StdSet.h"
#include "common/stdvector.h"
#include "deskflow/KeyState.h"

#include <Carbon/Carbon.h>

class IOSXKeyResource;

//! OS X key state
/*!
A key state for OS X.
*/
class OSXKeyState : public KeyState
{
public:
  using KeyIDs = std::vector<KeyID>;

  OSXKeyState(IEventQueue *events, std::vector<std::string> layouts, bool isLangSyncEnabled);
  OSXKeyState(IEventQueue *events, deskflow::KeyMap &keyMap, std::vector<std::string> layouts, bool isLangSyncEnabled);
  virtual ~OSXKeyState();

  //! @name modifiers
  //@{

  //! Handle modifier key change
  /*!
  Determines which modifier keys have changed and updates the modifier
  state and sends key events as appropriate.
  */
  void handleModifierKeys(void *target, KeyModifierMask oldMask, KeyModifierMask newMask);

  //@}
  //! @name accessors
  //@{

  //! Convert OS X modifier mask to deskflow mask
  /*!
  Returns the deskflow modifier mask corresponding to the OS X modifier
  mask in \p mask.
  */
  KeyModifierMask mapModifiersFromOSX(uint32_t mask) const;

  //! Convert CG flags-style modifier mask to old-style Carbon
  /*!
  Still required in a few places for translation calls.
  */
  KeyModifierMask mapModifiersToCarbon(uint32_t mask) const;

  //! Map key event to keys
  /*!
  Converts a key event into a sequence of KeyIDs and the shadow modifier
  state to a modifier mask.  The KeyIDs list, in order, the characters
  generated by the key press/release.  It returns the id of the button
  that was pressed or released, or 0 if the button doesn't map to a known
  KeyID.
  */
  KeyButton mapKeyFromEvent(KeyIDs &ids, KeyModifierMask *maskOut, CGEventRef event) const;

  //! Map key and mask to native values
  /*!
  Calculates mac virtual key and mask for a key \p key and modifiers
  \p mask.  Returns \c true if the key can be mapped, \c false otherwise.
  */
  bool
  mapDeskflowHotKeyToMac(KeyID key, KeyModifierMask mask, uint32_t &macVirtualKey, uint32_t &macModifierMask) const;

  //@}

  // IKeyState overrides
  virtual bool fakeCtrlAltDel();
  virtual bool fakeMediaKey(KeyID id);
  virtual KeyModifierMask pollActiveModifiers() const;
  virtual int32_t pollActiveGroup() const;
  virtual void pollPressedKeys(KeyButtonSet &pressedKeys) const;

  CGEventFlags getModifierStateAsOSXFlags() const;

protected:
  // KeyState overrides
  virtual void getKeyMap(deskflow::KeyMap &keyMap);
  virtual void fakeKey(const Keystroke &keystroke);

private:
  class KeyResource;

  // Add hard coded special keys to a deskflow::KeyMap.
  void getKeyMapForSpecialKeys(deskflow::KeyMap &keyMap, int32_t group) const;

  // Convert keyboard resource to a key map
  bool getKeyMap(deskflow::KeyMap &keyMap, int32_t group, const IOSXKeyResource &r) const;

  // Get the available keyboard groups
  bool getGroups(AutoCFArray &) const;

  // Change active keyboard group to group
  void setGroup(int32_t group);

  // Send an event for the given modifier key
  void handleModifierKey(void *target, uint32_t virtualKey, KeyID id, bool down, KeyModifierMask newMask);

  // Checks if any in \p ids is a glyph key and if \p isCommand is false.
  // If so it adds the AltGr modifier to \p mask.  This allows OS X
  // servers to use the option key both as AltGr and as a modifier.  If
  // option is acting as AltGr (i.e. it generates a glyph and there are
  // no command modifiers active) then we don't send the super modifier
  // to clients because they'd try to match it as a command modifier.
  void adjustAltGrModifier(const KeyIDs &ids, KeyModifierMask *mask, bool isCommand) const;

  // Maps an OS X virtual key id to a KeyButton.  This simply remaps
  // the ids so we don't use KeyButton 0.
  static KeyButton mapVirtualKeyToKeyButton(uint32_t keyCode);

  // Maps a KeyButton to an OS X key code.  This is the inverse of
  // mapVirtualKeyToKeyButton.
  static uint32_t mapKeyButtonToVirtualKey(KeyButton keyButton);

  void init();

  // Post a key event to HID manager. It posts an event to HID client, a
  // much lower level than window manager which's the target from carbon
  // CGEventPost
  kern_return_t postHIDVirtualKey(uint8_t virtualKeyCode, bool postDown);

  // Get keyboard event flags accorfing to keyboard modifiers
  CGEventFlags getKeyboardEventFlags() const;
  CGEventFlags getDeviceDependedFlags() const;

  void setKeyboardModifiers(CGKeyCode virtualKey, bool keyDown);

  void postKeyboardKey(CGKeyCode virtualKey, bool keyDown);

private:
  // OS X uses a physical key if 0 for the 'A' key.  deskflow reserves
  // KeyButton 0 so we offset all OS X physical key ids by this much
  // when used as a KeyButton and by minus this much to map a KeyButton
  // to a physical button.
  enum
  {
    KeyButtonOffset = 1
  };

  using GroupMap = std::map<CFDataRef, int32_t>;
  using VirtualKeyMap = std::map<uint32_t, KeyID>;

  VirtualKeyMap m_virtualKeyMap;
  mutable uint32_t m_deadKeyState;
  AutoCFArray m_groups{nullptr, CFRelease};
  GroupMap m_groupMap;
  bool m_shiftPressed;
  bool m_controlPressed;
  bool m_altPressed;
  bool m_superPressed;
  bool m_capsPressed;
};
