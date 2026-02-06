/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2003 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Coordinate.h"
#include "common/Settings.h"
#include "deskflow/MouseTypes.h"

//! Secondary screen interface
/*!
This interface defines the methods common to all platform dependent
secondary screen implementations.
*/
class ISecondaryScreen
{
public:
  ISecondaryScreen()
  {
    m_invertScroll = Settings::value(Settings::Client::InvertScrollDirection).toBool();
    m_scrollScale = std::clamp(Settings::value(Settings::Client::YScrollScale).toDouble(), 0.1, 10.0);
  }

  virtual ~ISecondaryScreen() = default;
  //! @name accessors
  //@{

  //! Fake mouse press/release
  /*!
  Synthesize a press or release of mouse button \c id.
  */
  virtual void fakeMouseButton(ButtonID id, bool press) = 0;

  //! Fake mouse move
  /*!
  Synthesize a mouse move to the absolute coordinates \c x,y.
  */
  virtual void fakeMouseMove(int32_t x, int32_t y) = 0;

  //! Fake mouse move
  /*!
  Synthesize a mouse move to the relative coordinates \c dx,dy.
  */
  virtual void fakeMouseRelativeMove(int32_t dx, int32_t dy) const = 0;

  /**
   * @brief Synthesize a mouse wheel event of amount
   * This Implmentation for this method should call `applyScrollModifier` before sending the final delta to the system
   * @param delta the raw delta to fake
   */
  virtual void fakeMouseWheel(ScrollDelta delta) const = 0;

  /**
   * @brief Applies any scroll modfifers to the provided delta, This should only be done inside the subclasses
   * fakeMouseWheel impl
   * @param delta a ScrollDelta to be modified
   * @return the delta with inversion and scale applied
   */
  ScrollDelta applyScrollModifier(ScrollDelta delta) const
  {
    delta.y = static_cast<int32_t>(m_invertScroll ? delta.y * -m_scrollScale : delta.y * m_scrollScale);
    return delta;
  }

private:
  /**
   * @brief this member is used to modify the scroll direction.
   * It is used in the applyScrollModifier method
   */
  bool m_invertScroll = false;

  /**
   * @brief this member is used to modify the scroll scale.
   * It is used in the applyScrollModifier method
   */
  double m_scrollScale = 1.0;
  //@}
};
