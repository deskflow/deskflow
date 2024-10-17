/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2022 Red Hat, Inc.
 * Copyright (C) 2024 Symless Ltd.
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
