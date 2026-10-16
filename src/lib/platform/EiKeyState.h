/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/KeyState.h"

#include <cstdint>
#include <functional>
#include <string>
#include <xkbcommon/xkbcommon.h>

struct xkb_context;
struct xkb_keymap;
struct xkb_state;

namespace deskflow {

/// An xkbcommon key state, used by the Ei and wlroots computers
class EiKeyState : public KeyState
{
public:
  //! Injects an evdev keycode into the display server
  using FakeKeyFn = std::function<void(std::uint32_t keycode, bool isDown)>;

  EiKeyState(FakeKeyFn fakeKey, IEventQueue *events);
  ~EiKeyState() override;

  void init(int fd, std::size_t len);
  void initDefaultKeymap();

  // IKeyState overrides
  bool fakeCtrlAltDel() override;
  KeyModifierMask pollActiveModifiers() const override;
  std::int32_t pollActiveGroup() const override;
  void pollPressedKeys(KeyButtonSet &pressedKeys) const override;
  KeyID mapKeyFromKeyval(std::uint32_t keyval) const;
  void updateXkbState(std::uint32_t keyval, bool isPressed);
  void updateLockedModifiers(xkb_mod_mask_t lockedMods);
  void clearStaleModifiers() override;

  //! The current keymap, in XKB text format
  std::string keymapAsString() const;
  //! True if the key (xkb keycode) should auto-repeat
  bool keyRepeats(std::uint32_t keyval) const;
  void serializeModifiers(
      std::uint32_t &depressed, std::uint32_t &latched, std::uint32_t &locked, std::uint32_t &group
  ) const;

protected:
  // KeyState overrides
  void getKeyMap(KeyMap &keyMap) override;
  void fakeKey(const Keystroke &keystroke) override;

private:
  std::uint32_t convertModMask(xkb_mod_mask_t xkbModMaskIn, bool mapMod2ToNumLock = false) const;
  void assignGeneratedModifiers(std::uint32_t keycode, KeyMap::KeyItem &item);

  FakeKeyFn m_fakeKey;

  xkb_context *m_xkb = nullptr;
  xkb_keymap *m_xkbKeymap = nullptr;
  xkb_state *m_xkbState = nullptr;
};

} // namespace deskflow
