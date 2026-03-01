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
  store_dead,
  keep_pending,
  consume_with_composed_char,
  consume_with_dead_fallback,
  consume_before_new_candidate,
  no_dead_interaction
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
  DeadKeyAction m_action = DeadKeyAction::no_dead_interaction;
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

  if (isKeyDown && input.m_toUnicodeResult < 0) {
    return {DeadKeyAction::store_dead, false, true, false, false, false};
  }

  if (!input.m_hasPendingDead) {
    return {DeadKeyAction::no_dead_interaction, false, false, false, false, false};
  }

  if (!isKeyDown) {
    if (input.m_deadOwnerButton != 0 && input.m_button == input.m_deadOwnerButton) {
      return {DeadKeyAction::consume_with_dead_fallback, false, true, true, true, false};
    }
    return {DeadKeyAction::keep_pending, false, false, false, false, false};
  }

  if (input.m_isModifierOrToggle) {
    return {DeadKeyAction::keep_pending, false, false, false, false, false};
  }

  if (input.m_toUnicodeResult == 1) {
    return {DeadKeyAction::consume_with_composed_char, false, true, true, false, false};
  }

  if (input.m_toUnicodeResult == 2) {
    return {DeadKeyAction::consume_with_dead_fallback, false, true, true, true, false};
  }

  if (input.m_deadOwnerButton == 0) {
    return {DeadKeyAction::keep_pending, true, false, false, false, false};
  }

  if (input.m_deadOwnerButton == input.m_button) {
    return {DeadKeyAction::keep_pending, false, false, false, false, false};
  }

  return {DeadKeyAction::consume_before_new_candidate, false, true, true, true, true};
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
