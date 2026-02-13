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
    m_invertYScroll = Settings::value(Settings::Client::InvertYScroll).toBool();
    m_yScrollScale = std::clamp(Settings::value(Settings::Client::YScrollScale).toDouble(), 0.1, 10.0);
    m_invertXScroll = Settings::value(Settings::Client::InvertXScroll).toBool();
    m_xScrollScale = std::clamp(Settings::value(Settings::Client::XScrollScale).toDouble(), 0.1, 10.0);
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
    delta.y = static_cast<int32_t>(m_invertYScroll ? delta.y * -m_yScrollScale : delta.y * m_yScrollScale);
    delta.x = static_cast<int32_t>(m_invertXScroll ? delta.x * -m_xScrollScale : delta.x * m_xScrollScale);
    return delta;
  }

private:
  /**
   * @brief this member is used to modify the verical scroll direction.
   * It is used in the applyScrollModifier method
   */
  bool m_invertYScroll = false;

  /**
   * @brief this member is used to modify the vertical scroll scale.
   * It is used in the applyScrollModifier method
   */
  double m_yScrollScale = 1.0;

  /**
   * @brief this member is used to modify the horizontal scroll direction.
   * It is used in the applyScrollModifier method
   */
  bool m_invertXScroll = false;

  /**
   * @brief this member is used to modify the horizontal scroll scale.
   * It is used in the applyScrollModifier method
   */
  double m_xScrollScale = 1.0;
  //@}
};
