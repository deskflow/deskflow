/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <Windows.h>

namespace deskflow {
namespace win32 {

struct DeadKeyState
{
  WPARAM m_deadVirtKey = 0;
  LPARAM m_deadLParam = 0;
  WCHAR m_deadChar = 0;
  BYTE m_deadKeyState[256] = {0};
};

enum class DeadKeyEventType
{
  KeyDown,
  KeyUp
};

enum class DeadKeyAction
{
  StoreDead,
  KeepPending,
  ConsumeWithComposedChar,
  ConsumeWithDeadFallback,
  ConsumeBeforeNewCandidate,
  NoDeadInteraction
};

struct DeadKeyTransitionInput
{
  DeadKeyEventType m_eventType = DeadKeyEventType::KeyDown;
  int m_toUnicodeResult = 0;
  UINT m_vk = 0;
  UINT m_button = 0;
  bool m_isModifierOrToggle = false;
  bool m_hasPendingDead = false;
  UINT m_deadOwnerButton = 0;
};

struct DeadKeyTransition
{
  DeadKeyAction m_action = DeadKeyAction::NoDeadInteraction;
  bool m_setOwner = false;
  bool m_clearOwner = false;
  bool m_clearPending = false;
  bool m_emitDeadFallback = false;
  bool m_recomputeWithoutDead = false;
};

inline bool isModifierOrToggleVK(UINT vk)
{
  switch (vk) {
  case VK_SHIFT:
  case VK_LSHIFT:
  case VK_RSHIFT:
  case VK_CONTROL:
  case VK_LCONTROL:
  case VK_RCONTROL:
  case VK_MENU:
  case VK_LMENU:
  case VK_RMENU:
  case VK_LWIN:
  case VK_RWIN:
  case VK_CAPITAL:
  case VK_NUMLOCK:
  case VK_SCROLL:
    return true;

  default:
    return false;
  }
}

inline void sanitizeDeadKeyState(BYTE keys[256], UINT deadVK)
{
  BYTE sanitized[256] = {0};
  for (UINT i = 0; i < 256; ++i) {
    if (i == deadVK || isModifierOrToggleVK(i)) {
      sanitized[i] = keys[i];
    }
  }

  for (size_t i = 0; i < 256; ++i) {
    keys[i] = sanitized[i];
  }
}

inline bool shouldClearDeadKeyState(int toUnicodeResult, bool isKeyUpEvent)
{
  return (toUnicodeResult == 1 || toUnicodeResult == 2) && !isKeyUpEvent;
}

inline DeadKeyTransition decideDeadKeyTransition(const DeadKeyTransitionInput &input)
{
  const bool isKeyDown = (input.m_eventType == DeadKeyEventType::KeyDown);
  const bool hasPendingDead = input.m_hasPendingDead;
  const bool isModifierOrToggle = input.m_isModifierOrToggle;
  const bool isOwnerButton = (input.m_deadOwnerButton != 0 && input.m_button == input.m_deadOwnerButton);

  // Priority 1: a fresh dead key candidate was pressed.
  // Store it immediately so subsequent key events can consume or fallback it.
  // NOTE: order matters; this check must run before "no pending dead".
  if (isKeyDown && input.m_toUnicodeResult < 0) {
    return {DeadKeyAction::StoreDead, false, true, false, false, false};
  }

  // Priority 2: no dead key is pending, so this event does not interact with dead-key state.
  if (!hasPendingDead) {
    return {DeadKeyAction::NoDeadInteraction, false, false, false, false, false};
  }

  // Priority 3: key-up handling while a dead key is pending.
  // If this key-up belongs to the owner button, force fallback and clear;
  // otherwise keep pending for the next key-down.
  if (!isKeyDown) {
    if (isOwnerButton) {
      return {DeadKeyAction::ConsumeWithDeadFallback, false, true, true, true, false};
    }
    return {DeadKeyAction::KeepPending, false, false, false, false, false};
  }

  // Priority 4: modifiers/toggles do not consume pending dead keys.
  if (isModifierOrToggle) {
    return {DeadKeyAction::KeepPending, false, false, false, false, false};
  }

  // Priority 5: key-down composition outcomes.
  // result == 1: composed char produced (consume pending dead key).
  if (input.m_toUnicodeResult == 1) {
    return {DeadKeyAction::ConsumeWithComposedChar, false, true, true, false, false};
  }

  // result == 2: pending dead key did not compose with current key.
  // Emit fallback for dead key and continue with current key.
  if (input.m_toUnicodeResult == 2) {
    return {DeadKeyAction::ConsumeWithDeadFallback, false, true, true, true, false};
  }

  // Priority 6: track ownership for overlap scenarios.
  if (input.m_deadOwnerButton == 0) {
    return {DeadKeyAction::KeepPending, true, false, false, false, false};
  }

  if (isOwnerButton) {
    return {DeadKeyAction::KeepPending, false, false, false, false, false};
  }

  // Priority 7: overlap with a different key/button while dead is pending.
  // Consume pending dead fallback first, clear dead state, then recompute.
  return {DeadKeyAction::ConsumeBeforeNewCandidate, false, true, true, true, true};
}

inline void captureDeadKeyState(
    DeadKeyState &state, WPARAM deadVirtKey, LPARAM deadLParam, const BYTE keys[256], WCHAR deadChar = 0
)
{
  state.m_deadVirtKey = deadVirtKey;
  state.m_deadLParam = deadLParam;
  state.m_deadChar = deadChar;
  for (size_t i = 0; i < 256; ++i) {
    state.m_deadKeyState[i] = keys[i];
  }
  sanitizeDeadKeyState(state.m_deadKeyState, static_cast<UINT>(deadVirtKey));
}

inline void clearDeadKeyState(DeadKeyState &state)
{
  state.m_deadVirtKey = 0;
  state.m_deadLParam = 0;
  state.m_deadChar = 0;
}

} // namespace win32
} // namespace deskflow
