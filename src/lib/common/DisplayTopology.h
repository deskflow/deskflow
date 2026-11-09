// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QByteArray>
#include <QLineF>
#include <QMap>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>
#include <optional>

namespace deskflow::display {
// Input coordinates are the coordinates the existing platform core accepts.
// Logical geometry and current pixel resolution are separate, especially on macOS.
struct Monitor
{
  QString id;
  QRectF logical;
  QRect input;
  QSize pixels;
  std::optional<QSizeF> physicalMm;
  bool operator==(const Monitor &) const = default;
};
struct Topology
{
  QVector<Monitor> monitors;
  QString error() const;
  QByteArray encode() const;
  QByteArray revision() const;
  static std::optional<Topology> decode(const QByteArray &);
  const Monitor *find(const QString &) const;
  bool operator==(const Topology &) const = default;
};
// Read-only native snapshot. Unsupported or unreliable identity/geometry yields
// an empty topology, never invented display IDs or physical dimensions.
Topology localTopology();

struct Placement
{
  QString device, monitor;
  QPointF position;
  // Explicit application-layout units. Absent in schema 1, which used each
  // monitor's logical size. This never changes native input geometry or DPI.
  std::optional<QSizeF> extent;
  bool operator==(const Placement &) const = default;
};
using Topologies = QMap<QString, Topology>;
Topologies decodeTopologies(const QByteArray &);
QByteArray encodeTopologies(const Topologies &);
enum class Side
{
  Left,
  Right,
  Top,
  Bottom
};
struct Crossing
{
  QString device, monitor;
  QPoint input;
  Side side;
};
struct Route
{
  QString device, monitor, targetDevice, targetMonitor;
  Side side;
  double sourceStart, sourceEnd, targetStart, targetEnd;
  QLineF segment;
};
struct Layout
{
  QVector<Placement> placements;
  QMap<QString, QByteArray> revisions; // Explicit saved snapshots; changed geometry requires review.
  QByteArray encode() const;
  static std::optional<Layout> decode(const QByteArray &);
  QString error(const Topologies &) const;
  // Structural/geometry validation without requiring all live components to
  // remain connected after a peer disconnects. New plans still use error().
  QString geometryError(const Topologies &) const;
  Layout available(const Topologies &) const;
  QRectF canvasRect(const Topologies &, const Placement &) const;
  QVector<Route> routes(const Topologies &) const;
  // Maps an exposed source monitor edge to a shared canvas edge. Missing peers,
  // gaps, local adjoining displays and corner-only contact never create links.
  std::optional<Crossing>
  cross(const Topologies &, const QString &device, const QString &monitor, Side, QPointF input, int inset = 1) const;
  // Core caches routes only after error() succeeds for these exact snapshots.
  std::optional<Crossing> crossValidated(
      const Topologies &, const QVector<Route> &, const QString &device, const QString &monitor, Side, QPointF input,
      int inset = 1
  ) const;
  std::optional<QString> monitorAt(const Topologies &, const QString &device, QPointF input) const;
  std::optional<Side> leaving(
      const Topologies &, const QString &device, const QString &monitor, QPointF previous, QPointF current, int zone = 0
  ) const;
  QPoint clamp(const Topologies &, const QString &device, const QString &monitor, QPointF input) const;
};
} // namespace deskflow::display
