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

protected:
  // KeyState overrides
  void getKeyMap(KeyMap &keyMap) override;
  void fakeKey(const Keystroke &keystroke) override;

private:
  std::uint32_t convertModMask(std::uint32_t xkbMask) const;
  void assignGeneratedModifiers(std::uint32_t keycode, KeyMap::KeyItem &item);

  EiScreen *m_screen = nullptr;

  xkb_context *m_xkb = nullptr;
  xkb_keymap *m_xkbKeymap = nullptr;
  xkb_state *m_xkbState = nullptr;
};

} // namespace deskflow
