// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "MonitorTopology.h"
#include <QGuiApplication>
#include <QRegion>
#include <QScreen>
#include <cmath>
namespace deskflow::gui {
MonitorTopology MonitorTopology::inspect(const QVector<MonitorGeometry> &input)
{
  MonitorTopology result;
  if (input.isEmpty() || input.size() > 16) {
    result.error = "invalid-monitor-count";
    return result;
  }
  QRegion region;
  for (const auto &m : input) {
    if (m.logical.width() < 1 || m.logical.height() < 1 || m.logical.width() > 100000 || m.logical.height() > 100000 ||
        std::abs(qint64(m.logical.x())) > 1000000 || std::abs(qint64(m.logical.y())) > 1000000 ||
        !std::isfinite(m.scale) || m.scale < 0.5 || m.scale > 8) {
      result.error = "invalid-monitor-geometry";
      return result;
    }
    bool mirror = false;
    for (const auto &previous : result.monitors) {
      if (previous.logical == m.logical)
        mirror = true;
      else if (previous.logical.intersects(m.logical))
        result.hasOverlap = true;
      if (std::abs(previous.scale - m.scale) > 0.001)
        result.mixedScale = true;
    }
    if (mirror) {
      result.mirrored = true;
      continue;
    }
    result.monitors.append(m);
    result.bounds = result.bounds.united(m.logical);
    region += m.logical;
  }
  result.hasGaps = region != QRegion(result.bounds);
  // ClientProxy1_0 reads DINF width/height into int16_t despite the protocol
  // comment saying unsigned. Do not promise support beyond that implementation.
  result.legacyRangeExceeded = result.bounds.x() < -32768 || result.bounds.y() < -32768 ||
                               result.bounds.right() > 32767 || result.bounds.bottom() > 32767 ||
                               result.bounds.width() > 32767 || result.bounds.height() > 32767;
  return result;
}
MonitorTopology MonitorTopology::local()
{
  QVector<MonitorGeometry> monitors;
  for (const auto *screen : QGuiApplication::screens())
    monitors.append({screen->geometry(), screen->devicePixelRatio()});
  return inspect(monitors);
}
} // namespace deskflow::gui
