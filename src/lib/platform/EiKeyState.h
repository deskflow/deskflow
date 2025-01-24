/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/KeyState.h"
#include "platform/EiScreen.h"

struct xkb_context;
struct xkb_keymap;
struct xkb_state;

namespace deskflow {

/// A key state for Ei
class EiKeyState : public KeyState
{
public:
  EiKeyState(EiScreen *screen, IEventQueue *events);
  ~EiKeyState();

  void init(int fd, std::size_t len);
  void init_default_keymap();

  // IKeyState overrides
  bool fakeCtrlAltDel() override;
  KeyModifierMask pollActiveModifiers() const override;
  std::int32_t pollActiveGroup() const override;
  void pollPressedKeys(KeyButtonSet &pressedKeys) const override;
  KeyID map_key_from_keyval(std::uint32_t keyval) const;
  void update_xkb_state(std::uint32_t keyval, bool is_pressed);

protected:
  // KeyState overrides
  void getKeyMap(KeyMap &keyMap) override;
  void fakeKey(const Keystroke &keystroke) override;

private:
  std::uint32_t convert_mod_mask(std::uint32_t xkb_mask) const;
  void assign_generated_modifiers(std::uint32_t keycode, KeyMap::KeyItem &item);

  EiScreen *screen_ = nullptr;

  xkb_context *xkb_ = nullptr;
  xkb_keymap *xkb_keymap_ = nullptr;
  xkb_state *xkb_state_ = nullptr;
};

} // namespace deskflow
