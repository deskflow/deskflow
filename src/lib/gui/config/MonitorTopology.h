// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QRect>
#include <QVector>
namespace deskflow::gui {
struct MonitorGeometry
{
  QRect logical;
  double scale = 1;
};
struct MonitorTopology
{
  QVector<MonitorGeometry> monitors;
  QRect bounds;
  QString error;
  bool hasGaps = false, hasOverlap = false, mixedScale = false, mirrored = false, legacyRangeExceeded = false;
  bool rectangularAggregate() const
  {
    return error.isEmpty() && !hasGaps && !hasOverlap && !mixedScale && !legacyRangeExceeded;
  }
  static MonitorTopology inspect(const QVector<MonitorGeometry> &monitors);
  static MonitorTopology local(); // Qt geometry only: no screen pixels, titles, serials or EDID.
};
} // namespace deskflow::gui
