/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Coordinate.h"

#include <cmath>

namespace deskflow {

// Accumulate sub-notch input before forwarding whole wheel clicks to clients.
class EiScrollState
{
public:
  ScrollDelta smooth(double dx, double dy)
  {
    // Preserve the existing scale: ten pixels make one wheel click.
    return {accumulate(m_smoothX, dx * 0.1, 1), accumulate(m_smoothY, dy * 0.1, 1)};
  }

  ScrollDelta discrete(int32_t dx, int32_t dy)
  {
    return {accumulate(m_discreteX, dx, s_scrollDelta), accumulate(m_discreteY, dy, s_scrollDelta)};
  }

  void reset(bool x = true, bool y = true)
  {
    if (x)
      m_smoothX = m_discreteX = 0;
    if (y)
      m_smoothY = m_discreteY = 0;
  }

private:
  static constexpr int32_t s_scrollDelta = 120;

  static int32_t accumulate(double &remainder, double delta, double unitsPerClick)
  {
    // Old fractional motion must not cancel the first notch after reversing.
    if ((delta < 0 && remainder > 0) || (delta > 0 && remainder < 0))
      remainder = 0;
    const auto total = remainder + delta;
    // Truncate toward zero so a negative fraction does not emit a full click.
    const auto clicks = std::trunc(total / unitsPerClick);
    remainder = total - clicks * unitsPerClick;
    return static_cast<int32_t>(clicks) * s_scrollDelta;
  }

  // Smooth remainders are fractional clicks; discrete remainders are v120
  // units. A compositor may send both event types on the same device.
  double m_smoothX = 0;
  double m_smoothY = 0;
  double m_discreteX = 0;
  double m_discreteY = 0;
};

} // namespace deskflow
