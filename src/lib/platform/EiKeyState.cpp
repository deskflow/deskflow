/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Devs.
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiKeyState.h"

#include "base/Log.h"
#include "deskflow/AppUtil.h"
#include "deskflow/ClientApp.h"
#include "platform/XWindowsUtil.h"

#include <cstddef>
#include <memory>
#include <unistd.h>

namespace deskflow {

EiKeyState::EiKeyState(EiScreen *screen, IEventQueue *events)
    : KeyState(events, AppUtil::instance().getKeyboardLayoutList(), ClientApp::instance().args().m_enableLangSync),
      m_screen{screen}
{
  m_xkb = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

  // FIXME: PrimaryClient->enable() calls into our keymap, so we must have
  // one during initial startup - even before we know what our actual keymap is.
  // Once we get the actual keymap from EIS, we swap it out so hopefully that's
  // enough.
  initDefaultKeymap();
}

void EiKeyState::initDefaultKeymap()
{
  if (m_xkbKeymap) {
    xkb_keymap_unref(m_xkbKeymap);
  }
  m_xkbKeymap = xkb_keymap_new_from_names(m_xkb, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS);

  if (m_xkbState) {
    xkb_state_unref(m_xkbState);
  }
  m_xkbState = xkb_state_new(m_xkbKeymap);
}

void EiKeyState::init(int fd, size_t len)
{
  auto buffer = std::make_unique<char[]>(len + 1);
  lseek(fd, 0, SEEK_SET);

  if (auto sz = read(fd, buffer.get(), len); (size_t)sz < len) {
    LOG_WARN("failed to create xkb context: %s", strerror(errno));
    return;
  }

  // See xkbcommon/libxkbcommon issue #307, xkb_keymap_new_from_buffer fails if
  // we have a terminating null byte. Since we can't control whether the other
  // end sends that byte, enforce null-termination in our buffer and pass the
  // whole thing as string.

  buffer[len] = '\0'; // guarantee null-termination
  auto keymap = xkb_keymap_new_from_string(m_xkb, buffer.get(), XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!keymap) {
    LOG_WARN("failed to compile keymap, falling back to defaults");
    // Falling back to layout "us" is a lot more useful than segfaulting
    initDefaultKeymap();
    return;
  }

  if (m_xkbKeymap) {
    xkb_keymap_unref(m_xkbKeymap);
  }
  m_xkbKeymap = keymap;

  if (m_xkbState) {
    xkb_state_unref(m_xkbState);
  }
  m_xkbState = xkb_state_new(m_xkbKeymap);
}

EiKeyState::~EiKeyState()
{
  xkb_context_unref(m_xkb);
  xkb_keymap_unref(m_xkbKeymap);
  xkb_state_unref(m_xkbState);
}

bool EiKeyState::fakeCtrlAltDel()
{
  // pass keys through unchanged
  return false;
}

KeyModifierMask EiKeyState::pollActiveModifiers() const
{
  const auto xkbMask = xkb_state_serialize_mods(m_xkbState, XKB_STATE_MODS_EFFECTIVE);
  return convertModMask(xkbMask);
}

std::int32_t EiKeyState::pollActiveGroup() const
{
  return xkb_state_serialize_layout(m_xkbState, XKB_STATE_LAYOUT_EFFECTIVE);
}

void EiKeyState::pollPressedKeys(KeyButtonSet &pressedKeys) const
{
  // FIXME
  return;
}

std::uint32_t EiKeyState::convertModMask(xkb_mod_mask_t xkbModMaskIn) const
{
  // This is our own modifier mask, not xkb's.
  std::uint32_t modMaskOut = 0;

  for (xkb_mod_index_t xkbModIdx = 0; xkbModIdx < xkb_keymap_num_mods(m_xkbKeymap); xkbModIdx++) {
    const char *name = xkb_keymap_mod_get_name(m_xkbKeymap, xkbModIdx);

#ifdef HAVE_XKB_KEYMAP_MOD_GET_MASK
    // Available since xkbcommon v1.10
    // Note: xkb_keymap_mod_get_mask2 was added in v1.11 which accepts xkb_mod_index_t.
    const auto xkbModMask = xkb_keymap_mod_get_mask(m_xkbKeymap, name);
#else
    // HACK: in older xkbcommon we need to create the mask manually from the index.
    const xkb_mod_mask_t xkbModMask = (1 << xkbModIdx);
#endif

    // Skip inactive modifiers.
    if ((xkbModMaskIn & xkbModMask) == 0)
      continue;

    /* added in libxkbcommon 1.8.0 in the same commit so we have all or none */
#ifndef XKB_VMOD_NAME_ALT
    static const auto XKB_VMOD_NAME_ALT = "Alt";
    static const auto XKB_VMOD_NAME_LEVEL3 = "LevelThree";
    static const auto XKB_VMOD_NAME_LEVEL5 = "LevelFive";
    static const auto XKB_VMOD_NAME_META = "Meta";
    static const auto XKB_VMOD_NAME_NUM = "NumLock";
    static const auto XKB_VMOD_NAME_SCROLL = "ScrollLock";
    static const auto XKB_VMOD_NAME_SUPER = "Super";
    static const auto XKB_MOD_NAME_MOD2 = "Mod2";
    static const auto XKB_MOD_NAME_MOD3 = "Mod3";
    static const auto XKB_MOD_NAME_MOD5 = "Mod5";
#endif

    if (strcmp(XKB_MOD_NAME_SHIFT, name) == 0)
      modMaskOut |= (1 << kKeyModifierBitShift);
    else if (strcmp(XKB_MOD_NAME_CAPS, name) == 0)
      modMaskOut |= (1 << kKeyModifierBitCapsLock);
    else if (strcmp(XKB_MOD_NAME_CTRL, name) == 0)
      modMaskOut |= (1 << kKeyModifierBitControl);
    else if (strcmp(XKB_MOD_NAME_ALT, name) == 0 || strcmp(XKB_VMOD_NAME_ALT, name) == 0)
      modMaskOut |= (1 << kKeyModifierBitAlt);
    else if (strcmp(XKB_MOD_NAME_LOGO, name) == 0 || strcmp(XKB_VMOD_NAME_SUPER, name) == 0)
      modMaskOut |= (1 << kKeyModifierBitSuper);
    else if (strcmp(XKB_MOD_NAME_MOD5, name) == 0 || strcmp(XKB_VMOD_NAME_LEVEL3, name) == 0)
      modMaskOut |= (1 << kKeyModifierBitAltGr);
    else if (strcmp(XKB_VMOD_NAME_LEVEL5, name) == 0)
      modMaskOut |= (1 << kKeyModifierBitLevel5Lock);
    else if (strcmp(XKB_VMOD_NAME_META, name) == 0)
      modMaskOut |= (1 << kKeyModifierBitMeta);
    else if (strcmp(XKB_VMOD_NAME_NUM, name) == 0)
      modMaskOut |= (1 << kKeyModifierBitNumLock);
    else if (strcmp(XKB_VMOD_NAME_SCROLL, name) == 0)
      modMaskOut |= (1 << kKeyModifierBitScrollLock);
    else if (strcmp(XKB_MOD_NAME_MOD2, name) == 0) // spare, sometimes mapped to num lock.
      LOG_DEBUG2("modifier mask %s ignored", name);
    else if (strcmp(XKB_MOD_NAME_MOD3, name) == 0) // spare, could be mapped to alt_r, caps lock, scroll lock, etc.
      LOG_DEBUG2("modifier mask %s ignored", name);
    else
      LOG_WARN("modifier mask %s not accounted for, this is a bug", name);
  }

  return modMaskOut;
}

// Only way to figure out whether a key is a modifier key is to press it,
// check if a modifier changed state and then release it again.
// Luckily xkbcommon allows us to do this in a separate state.
void EiKeyState::assignGeneratedModifiers(std::uint32_t keycode, deskflow::KeyMap::KeyItem &item)
{
  xkb_mod_mask_t xkbModMask = 0;
  auto state = xkb_state_new(m_xkbKeymap);

  if (enum xkb_state_component changed = xkb_state_update_key(state, keycode, XKB_KEY_DOWN); changed) {
    for (xkb_mod_index_t m = 0; m < xkb_keymap_num_mods(m_xkbKeymap); m++) {
      if (xkb_state_mod_index_is_active(state, m, XKB_STATE_MODS_LOCKED))
        item.m_lock = true;

      if (xkb_state_mod_index_is_active(state, m, XKB_STATE_MODS_EFFECTIVE)) {
        xkbModMask |= (1 << m);
      }
    }
  }
  xkb_state_update_key(state, keycode, XKB_KEY_UP);
  xkb_state_unref(state);

  item.m_generates = convertModMask(xkbModMask);
}

void EiKeyState::getKeyMap(deskflow::KeyMap &keyMap)
{
  auto minKeycode = xkb_keymap_min_keycode(m_xkbKeymap);
  auto maxKeycode = xkb_keymap_max_keycode(m_xkbKeymap);

  // X keycodes are evdev keycodes + 8 (libei gives us evdev keycodes)
  for (auto keycode = minKeycode; keycode <= maxKeycode; keycode++) {

    // skip keys with no groups (they generate no symbols)
    if (xkb_keymap_num_layouts_for_key(m_xkbKeymap, keycode) == 0)
      continue;

    for (auto group = 0U; group < xkb_keymap_num_layouts(m_xkbKeymap); group++) {
      for (auto level = 0U; level < xkb_keymap_num_levels_for_key(m_xkbKeymap, keycode, group); level++) {
        const xkb_keysym_t *syms;
        xkb_mod_mask_t masks[64];
        auto nmasks = xkb_keymap_key_get_mods_for_level(m_xkbKeymap, keycode, group, level, masks, 64);
        auto nsyms = xkb_keymap_key_get_syms_by_level(m_xkbKeymap, keycode, group, level, &syms);

        if (nsyms == 0)
          continue;

        if (nsyms > 1)
          LOG_WARN("multiple keysyms per keycode are not supported, keycode %d", keycode);

        deskflow::KeyMap::KeyItem item{};
        xkb_keysym_t keysym = syms[0];
        KeySym sym = keysym;
        item.m_id = XWindowsUtil::mapKeySymToKeyID(sym);
        item.m_button = static_cast<KeyButton>(keycode) - 8; // X keycode offset
        item.m_group = group;

        // For debugging only
        char keysymName[128] = {0};
        xkb_keysym_get_name(keysym, keysymName, sizeof(keysymName));

        // Set to all modifiers this key may be affected by
        uint32_t modSensitive = 0;
        for (auto n = 0U; n < nmasks; n++) {
          modSensitive |= masks[n];
        }
        item.m_sensitive = convertModMask(modSensitive);

        uint32_t modRequired = 0;
        for (std::size_t m = 0; m < nmasks; m++) {
          modRequired |= masks[m];
        }
        item.m_required = convertModMask(modRequired);

        assignGeneratedModifiers(keycode, item);

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
  case Keystroke::KeyType::Button:
    LOG_DEBUG1(
        "fake key: %03x (%08x) %s", keystroke.m_data.m_button.m_button, keystroke.m_data.m_button.m_client,
        keystroke.m_data.m_button.m_press ? "down" : "up"
    );
    m_screen->fakeKey(keystroke.m_data.m_button.m_button, keystroke.m_data.m_button.m_press);
    break;
  default:
    break;
  }
}

KeyID EiKeyState::mapKeyFromKeyval(uint32_t keyval) const
{
  // FIXME: That might be a bit crude...?
  xkb_keysym_t xkbKeysym = xkb_state_key_get_one_sym(m_xkbState, keyval);
  auto keysym = static_cast<KeySym>(xkbKeysym);

  KeyID keyid = XWindowsUtil::mapKeySymToKeyID(keysym);
  LOG_DEBUG1("mapped key: code=%d keysym=0x%04lx to keyID=%d", keyval, keysym, keyid);

  return keyid;
}

void EiKeyState::updateXkbState(uint32_t keyval, bool isPressed)
{
  LOG_DEBUG1("update key state: keyval=%d pressed=%i", keyval, isPressed);
  xkb_state_update_key(m_xkbState, keyval, isPressed ? XKB_KEY_DOWN : XKB_KEY_UP);
}

} // namespace deskflow
