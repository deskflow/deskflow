/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>

/**
 * @brief Screen edge directions for mouse movement
 *
 * Used to specify which edge of a screen the mouse cursor crosses
 * when moving between primary and secondary screens.
 *
 * @since Protocol version 1.0
 */
enum class Direction : uint8_t
{
  NoDirection,                                                             ///< No specific direction
  Left,                                                                    ///< Left edge of screen
  Right,                                                                   ///< Right edge of screen
  Top,                                                                     ///< Top edge of screen
  Bottom,                                                                  ///< Bottom edge of screen
  FirstDirection = Direction::Left,                                        ///< First valid direction value
  LastDirection = Direction::Bottom,                                       ///< Last valid direction value
  NumDirections = Direction::LastDirection - Direction::FirstDirection + 1 ///< Total number of directions
};

/**
 * @brief Bitmask values for screen edge directions
 *
 * Used to create bitmasks representing multiple screen edges.
 * Useful for configuration and edge detection.
 *
 * @since Protocol version 1.0
 */
enum class DirectionMask
{
  NoDirMask = 0,                                        ///< No direction mask
  LeftMask = 1 << static_cast<int>(Direction::Left),    ///< Left edge mask
  RightMask = 1 << static_cast<int>(Direction::Right),  ///< Right edge mask
  TopMask = 1 << static_cast<int>(Direction::Top),      ///< Top edge mask
  BottomMask = 1 << static_cast<int>(Direction::Bottom) ///< Bottom edge mask
};
