/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <set>
#include "deskflow/KeyState.h"
#include "platform/EiComputer.h"

#include <xkbcommon/xkbcommon.h>

struct xkb_context;
struct xkb_keymap;
struct xkb_state;

namespace deskflow {

/// A key state for Ei
class EiKeyState : public KeyState
{
public:
  void fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang) override;
  bool fakeKeyRepeat(KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang) override;
  bool fakeKeyUp(KeyButton button) override;
  void fakeAllKeysUp() override;

  EiKeyState(EiComputer *computer, IEventQueue *events);
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

protected:
  // KeyState overrides
  void getKeyMap(KeyMap &keyMap) override;
  void fakeKey(const Keystroke &keystroke) override;

private:
  std::uint32_t convertModMask(xkb_mod_mask_t xkbModMaskIn, bool mapMod2ToNumLock = false) const;
  void assignGeneratedModifiers(std::uint32_t keycode, KeyMap::KeyItem &item);

  bool m_physicalKeys = false;
  std::set<uint32_t> m_physicalDown;
  EiComputer *m_computer = nullptr;

  xkb_context *m_xkb = nullptr;
  xkb_keymap *m_xkbKeymap = nullptr;
  xkb_state *m_xkbState = nullptr;
};

} // namespace deskflow
