/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiKeyState.h"

#include "base/Log.h"
#include "deskflow/AppUtil.h"
#include "deskflow/ClientApp.h"
#include "platform/XWindowsUtil.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

namespace deskflow {

EiKeyState::EiKeyState(EiScreen *screen, IEventQueue *events)
    : KeyState(events, AppUtil::instance().getKeyboardLayoutList(), ClientApp::instance().args().m_enableLangSync),
      screen_{screen}
{
  xkb_ = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

  // FIXME: PrimaryClient->enable() calls into our keymap, so we must have
  // one during initial startup - even before we know what our actual keymap is.
  // Once we get the actual keymap from EIS, we swap it out so hopefully that's
  // enough.
  init_default_keymap();
}

void EiKeyState::init_default_keymap()
{
  if (xkb_keymap_) {
    xkb_keymap_unref(xkb_keymap_);
  }
  xkb_keymap_ = xkb_keymap_new_from_names(xkb_, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS);

  if (xkb_state_) {
    xkb_state_unref(xkb_state_);
  }
  xkb_state_ = xkb_state_new(xkb_keymap_);
}

void EiKeyState::init(int fd, size_t len)
{
  auto buffer = std::make_unique<char[]>(len + 1);
  lseek(fd, 0, SEEK_SET);
  auto sz = read(fd, buffer.get(), len);

  if ((size_t)sz < len) {
    LOG_WARN("failed to create xkb context: %s", strerror(errno));
    return;
  }

  // See xkbcommon/libxkbcommon issue #307, xkb_keymap_new_from_buffer fails if
  // we have a terminating null byte. Since we can't control whether the other
  // end sends that byte, enforce null-termination in our buffer and pass the
  // whole thing as string.

  buffer[len] = '\0'; // guarantee null-termination
  auto keymap = xkb_keymap_new_from_string(xkb_, buffer.get(), XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!keymap) {
    LOG_WARN("failed to compile keymap, falling back to defaults");
    // Falling back to layout "us" is a lot more useful than segfaulting
    init_default_keymap();
    return;
  }

  if (xkb_keymap_) {
    xkb_keymap_unref(xkb_keymap_);
  }
  xkb_keymap_ = keymap;

  if (xkb_state_) {
    xkb_state_unref(xkb_state_);
  }
  xkb_state_ = xkb_state_new(xkb_keymap_);
}

EiKeyState::~EiKeyState()
{
  xkb_context_unref(xkb_);
  xkb_keymap_unref(xkb_keymap_);
  xkb_state_unref(xkb_state_);
}

bool EiKeyState::fakeCtrlAltDel()
{
  // pass keys through unchanged
  return false;
}

KeyModifierMask EiKeyState::pollActiveModifiers() const
{
  std::uint32_t xkb_mask = xkb_state_serialize_mods(xkb_state_, XKB_STATE_MODS_EFFECTIVE);
  return convert_mod_mask(xkb_mask);
}

std::int32_t EiKeyState::pollActiveGroup() const
{
  return xkb_state_serialize_layout(xkb_state_, XKB_STATE_LAYOUT_EFFECTIVE);
}

void EiKeyState::pollPressedKeys(KeyButtonSet &pressedKeys) const
{
  // FIXME
  return;
}

std::uint32_t EiKeyState::convert_mod_mask(std::uint32_t xkb_mask) const
{
  std::uint32_t barrier_mask = 0;

  for (xkb_mod_index_t xkbmod = 0; xkbmod < xkb_keymap_num_mods(xkb_keymap_); xkbmod++) {
    if ((xkb_mask & (1 << xkbmod)) == 0)
      continue;

    const char *name = xkb_keymap_mod_get_name(xkb_keymap_, xkbmod);
    if (strcmp(XKB_MOD_NAME_SHIFT, name) == 0)
      barrier_mask |= (1 << kKeyModifierBitShift);
    else if (strcmp(XKB_MOD_NAME_CAPS, name) == 0)
      barrier_mask |= (1 << kKeyModifierBitCapsLock);
    else if (strcmp(XKB_MOD_NAME_CTRL, name) == 0)
      barrier_mask |= (1 << kKeyModifierBitControl);
    else if (strcmp(XKB_MOD_NAME_ALT, name) == 0)
      barrier_mask |= (1 << kKeyModifierBitAlt);
    else if (strcmp(XKB_MOD_NAME_LOGO, name) == 0)
      barrier_mask |= (1 << kKeyModifierBitSuper);
  }

  return barrier_mask;
}

// Only way to figure out whether a key is a modifier key is to press it,
// check if a modifier changed state and then release it again.
// Luckily xkbcommon allows us to do this in a separate state.
void EiKeyState::assign_generated_modifiers(std::uint32_t keycode, deskflow::KeyMap::KeyItem &item)
{
  std::uint32_t mods_generates = 0;
  auto state = xkb_state_new(xkb_keymap_);
  enum xkb_state_component changed = xkb_state_update_key(state, keycode, XKB_KEY_DOWN);

  if (changed) {
    for (xkb_mod_index_t m = 0; m < xkb_keymap_num_mods(xkb_keymap_); m++) {
      if (xkb_state_mod_index_is_active(state, m, XKB_STATE_MODS_LOCKED))
        item.m_lock = true;

      if (xkb_state_mod_index_is_active(state, m, XKB_STATE_MODS_EFFECTIVE)) {
        mods_generates |= (1 << m);
      }
    }
  }
  xkb_state_update_key(state, keycode, XKB_KEY_UP);
  xkb_state_unref(state);

  item.m_generates = convert_mod_mask(mods_generates);
}

void EiKeyState::getKeyMap(deskflow::KeyMap &keyMap)
{
  auto min_keycode = xkb_keymap_min_keycode(xkb_keymap_);
  auto max_keycode = xkb_keymap_max_keycode(xkb_keymap_);

  // X keycodes are evdev keycodes + 8 (libei gives us evdev keycodes)
  for (auto keycode = min_keycode; keycode <= max_keycode; keycode++) {

    // skip keys with no groups (they generate no symbols)
    if (xkb_keymap_num_layouts_for_key(xkb_keymap_, keycode) == 0)
      continue;

    for (auto group = 0U; group < xkb_keymap_num_layouts(xkb_keymap_); group++) {
      for (auto level = 0U; level < xkb_keymap_num_levels_for_key(xkb_keymap_, keycode, group); level++) {
        const xkb_keysym_t *syms;
        xkb_mod_mask_t masks[64];
        auto nmasks = xkb_keymap_key_get_mods_for_level(xkb_keymap_, keycode, group, level, masks, 64);
        auto nsyms = xkb_keymap_key_get_syms_by_level(xkb_keymap_, keycode, group, level, &syms);

        if (nsyms == 0)
          continue;

        if (nsyms > 1)
          LOG_WARN("multiple keysyms per keycode are not supported, keycode %d", keycode);

        deskflow::KeyMap::KeyItem item{};
        xkb_keysym_t keysym = syms[0];
        KeySym sym = static_cast<KeyID>(keysym);
        item.m_id = XWindowsUtil::mapKeySymToKeyID(sym);
        item.m_button = static_cast<KeyButton>(keycode) - 8; // X keycode offset
        item.m_group = group;

        // For debugging only
        char keysym_name[128] = {0};
        xkb_keysym_get_name(keysym, keysym_name, sizeof(keysym_name));

        // Set to all modifiers this key may be affected by
        uint32_t mods_sensitive = 0;
        for (auto n = 0U; n < nmasks; n++) {
          mods_sensitive |= masks[n];
        }
        item.m_sensitive = convert_mod_mask(mods_sensitive);

        uint32_t mods_required = 0;
        for (std::size_t m = 0; m < nmasks; m++) {
          mods_required |= masks[m];
        }
        item.m_required = convert_mod_mask(mods_required);

        assign_generated_modifiers(keycode, item);

        // add capslock version of key is sensitive to capslock
        if (item.m_sensitive & KeyModifierShift && item.m_sensitive & KeyModifierCapsLock) {
          item.m_required &= ~KeyModifierShift;
          item.m_required |= KeyModifierCapsLock;
          keyMap.addKeyEntry(item);
          item.m_required |= KeyModifierShift;
          item.m_required &= ~KeyModifierCapsLock;
        }

        keyMap.addKeyEntry(item);
      }
    }
  }

  // allow composition across groups
  keyMap.allowGroupSwitchDuringCompose();
}

void EiKeyState::fakeKey(const Keystroke &keystroke)
{
  switch (keystroke.m_type) {
  case Keystroke::kButton:
    LOG_DEBUG1(
        "fake key: %03x (%08x) %s", keystroke.m_data.m_button.m_button, keystroke.m_data.m_button.m_client,
        keystroke.m_data.m_button.m_press ? "down" : "up"
    );
    screen_->fakeKey(keystroke.m_data.m_button.m_button, keystroke.m_data.m_button.m_press);
    break;
  default:
    break;
  }
}

KeyID EiKeyState::map_key_from_keyval(uint32_t keyval) const
{
  // FIXME: That might be a bit crude...?
  xkb_keysym_t xkb_keysym = xkb_state_key_get_one_sym(xkb_state_, keyval);
  KeySym keysym = static_cast<KeySym>(xkb_keysym);

  KeyID keyid = XWindowsUtil::mapKeySymToKeyID(keysym);
  LOG_DEBUG1("mapped key: code=%d keysym=0x%04lx to keyID=%d", keyval, keysym, keyid);

  return keyid;
}

void EiKeyState::update_xkb_state(uint32_t keyval, bool is_pressed)
{
  LOG_DEBUG1("update key state: keyval=%d pressed=%i", keyval, is_pressed);
  xkb_state_update_key(xkb_state_, keyval, is_pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
}

} // namespace deskflow
