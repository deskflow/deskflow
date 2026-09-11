/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXKeyLayoutResource.h"

// there are 128 virtual key codes and 32 modifier combinations (the
// right-handed modifier bits are ignored, matching the uchr parser).
static const uint32_t s_numButtons = 128;
static const uint32_t s_numModifierCombinations = 32;

OSXKeyLayoutResource::OSXKeyLayoutResource(const void *resource, uint32_t keyboardType)
    : m_layout(static_cast<const UCKeyboardLayout *>(resource)),
      m_keyboardType(keyboardType)
{
}

bool OSXKeyLayoutResource::isValid() const
{
  return m_layout != nullptr;
}

uint32_t OSXKeyLayoutResource::getNumModifierCombinations() const
{
  return s_numModifierCombinations;
}

uint32_t OSXKeyLayoutResource::getNumTables() const
{
  // one table per modifier combination
  return s_numModifierCombinations;
}

uint32_t OSXKeyLayoutResource::getNumButtons() const
{
  return s_numButtons;
}

uint32_t OSXKeyLayoutResource::getTableForModifier(uint32_t mask) const
{
  // the modifier combination is the table
  return (mask < s_numModifierCombinations) ? mask : 0;
}

KeyID OSXKeyLayoutResource::getKey(uint32_t table, uint32_t button) const
{
  if (m_layout == nullptr || button >= s_numButtons) {
    return kKeyNone;
  }

  // UCKeyTranslate's modifier key state uses the same low five bits as our
  // table index (cmd, shift, caps, option, control).
  const UInt32 modifierState = table;

  UInt32 deadKeyState = 0;
  UniChar chars[4] = {0};
  UniCharCount count = 0;
  OSStatus status = UCKeyTranslate(
      m_layout, static_cast<UInt16>(button), kUCKeyActionDown, modifierState, m_keyboardType, 0, &deadKeyState,
      sizeof(chars) / sizeof(chars[0]), &count, chars
  );
  if (status != noErr) {
    return kKeyNone;
  }

  if (count == 0) {
    // no output.  a non-zero dead-key state means this is a dead key: resolve
    // it to its spacing character by following it with a space, then convert
    // to the dead KeyID.
    if (deadKeyState == 0) {
      return kKeyNone;
    }
    return deskflow::KeyMap::getDeadKey(resolveDeadKeyOutput(deadKeyState));
  }

  // no support for multi-character output (matches the uchr parser).
  if (count != 1) {
    return kKeyNone;
  }

  return unicharToKeyID(chars[0]);
}

KeyID OSXKeyLayoutResource::getDeadKeyOutput(uint32_t table, uint32_t button) const
{
  if (m_layout == nullptr || button >= s_numButtons) {
    return kKeyNone;
  }

  UInt32 deadKeyState = 0;
  UniChar chars[4] = {0};
  UniCharCount count = 0;
  const OSStatus status = UCKeyTranslate(
      m_layout, static_cast<UInt16>(button), kUCKeyActionDown, table, m_keyboardType, 0, &deadKeyState,
      sizeof(chars) / sizeof(chars[0]), &count, chars
  );
  if (status != noErr || count != 0 || deadKeyState == 0) {
    return kKeyNone;
  }

  return resolveDeadKeyOutput(deadKeyState);
}

KeyID OSXKeyLayoutResource::resolveDeadKeyOutput(UInt32 deadKeyState) const
{
  UniChar chars[4] = {0};
  UniCharCount count = 0;
  const OSStatus status = UCKeyTranslate(
      m_layout, kVK_Space, kUCKeyActionDown, 0, m_keyboardType, 0, &deadKeyState, sizeof(chars) / sizeof(chars[0]),
      &count, chars
  );
  if (status != noErr || count != 1) {
    return kKeyNone;
  }

  return unicharToKeyID(chars[0]);
}
