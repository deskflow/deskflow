/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cmath>
#include <cstdint>

enum class NavigationGestureDirection : int8_t
{
  None = 0,
  Left,
  Right,
  Up,
  Down
};

enum class NavigationActionSlot : int8_t
{
  None = 0,
  Action1,
  Action2
};

enum class NavigationOutputAction : int
{
  Ignore = 0,
  Back,
  Forward,
  Keystroke
};

inline NavigationGestureDirection navigationGestureDirectionFromDeltas(double deltaX, double deltaY)
{
  const auto absX = std::abs(deltaX);
  const auto absY = std::abs(deltaY);
  if (absX == absY) {
    return NavigationGestureDirection::None;
  }
  if (absX > absY) {
    return deltaX > 0.0 ? NavigationGestureDirection::Left : NavigationGestureDirection::Right;
  }
  return deltaY > 0.0 ? NavigationGestureDirection::Up : NavigationGestureDirection::Down;
}
