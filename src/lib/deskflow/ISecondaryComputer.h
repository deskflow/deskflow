/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2003 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Coordinate.h"
#include "common/Settings.h"
#include "deskflow/MouseTypes.h"

//! Secondary computer interface
/*!
This interface defines the methods common to all platform dependent
secondary computer implementations.
*/
class ISecondaryComputer
{
public:
  ISecondaryComputer()
  {
    m_invertYScroll = Settings::value(Settings::Client::InvertYScroll).toBool();
    m_yScrollScale = std::clamp(Settings::value(Settings::Client::YScrollScale).toDouble(), 0.1, 10.0);
    m_invertXScroll = Settings::value(Settings::Client::InvertXScroll).toBool();
    m_xScrollScale = std::clamp(Settings::value(Settings::Client::XScrollScale).toDouble(), 0.1, 10.0);
  }

  virtual ~ISecondaryComputer() = default;
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
    // carry the fraction a scale leaves over, so tiny smooth-scroll deltas add up instead of truncating to 0
    const double y = (m_invertYScroll ? -m_yScrollScale : m_yScrollScale) * delta.y + m_yScaleRemainder;
    const double x = (m_invertXScroll ? -m_xScrollScale : m_xScrollScale) * delta.x + m_xScaleRemainder;
    delta.y = static_cast<int32_t>(y);
    delta.x = static_cast<int32_t>(x);
    m_yScaleRemainder = y - delta.y;
    m_xScaleRemainder = x - delta.x;
    return delta;
  }

  /**
   * @brief For clients that can only scroll in whole notches: holds back the sub-notch part of
   * smooth-scroll deltas until it adds up to a notch, instead of dropping or rounding it up
   * @param delta the delta after applyScrollModifier
   * @return whole multiples of s_scrollDelta
   */
  ScrollDelta takeWholeScrollNotches(ScrollDelta delta) const
  {
    // a reversed scroll starts fresh rather than first unwinding what was held for the other direction
    const auto add = [](int32_t &held, int32_t d) { held = (held < 0 && d > 0) || (held > 0 && d < 0) ? d : held + d; };
    add(m_notchRemainder.x, delta.x);
    add(m_notchRemainder.y, delta.y);
    const ScrollDelta notches{
        m_notchRemainder.x / s_scrollDelta * s_scrollDelta, m_notchRemainder.y / s_scrollDelta * s_scrollDelta
    };
    m_notchRemainder.x -= notches.x;
    m_notchRemainder.y -= notches.y;
    return notches;
  }

  // Delta for a "click"
  static const auto s_scrollDelta = 120;

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

  mutable double m_yScaleRemainder = 0.0;
  mutable double m_xScaleRemainder = 0.0;
  mutable ScrollDelta m_notchRemainder{0, 0};
  //@}
};
