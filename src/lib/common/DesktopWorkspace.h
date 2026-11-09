// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "DisplayTopology.h"
#include <QStringList>

namespace deskflow::display {

enum class Capability
{
  Unknown,
  Supported,
  Unsupported
};
enum class Connection
{
  Unknown,
  Live,
  Offline
};
enum class Observation
{
  Unknown,
  Observed,
  Cached,
  Unsupported,
  Unavailable
};
enum class ArrangementMode
{
  PreserveOsGroups,
  CustomScreenRoutes
};

// Capabilities and connection state are observations, never inferred from a
// friendly name, a cached snapshot, or a user-edited workspace file.
struct DeviceSnapshot
{
  QString id, displayName;
  Connection connection = Connection::Unknown;
  Observation observation = Observation::Unknown;
  Capability topologyCapability = Capability::Unknown;
  Capability routingCapability = Capability::Unknown;
  Capability identifyCapability = Capability::Unknown;
  bool local = false;
  bool authenticated = false;
  std::optional<Topology> topology;
  QString reason;
  QString identityReference; // Current verified certificate digest; never grants trust.
};
using DeviceSnapshots = QVector<DeviceSnapshot>;

struct WorkspaceState
{
  quint64 revision = 0;
  QMap<QString, QString> identityReferences; // protocol name -> already verified public certificate
  Layout layout;
  ArrangementMode mode = ArrangementMode::PreserveOsGroups;
  QMap<QString, int> displayNumbers;
  QMap<QString, QSizeF> physicalCalibration;
  QByteArray encode() const;
  static std::optional<WorkspaceState> decode(const QByteArray &);
};

struct WorkspaceDisplay
{
  QString deviceId, deviceName, stableId;
  int number = 0;
  QRect inputRect;
  QRectF logicalRect, canvasRect;
  QSize pixelResolution;
  QPointF inputScale, displayScale;
  std::optional<QSizeF> physicalMm;
  bool calibrated = false;
  bool cached = false;
};

// Read-only server view attached to the existing verified topology message.
// No authentication decisions or credentials are serialized in this object.
struct WorkspaceSnapshot
{
  QStringList devices, onlineDevices, topologyCapable;
  Topologies topologies;
  WorkspaceState applied;
  bool externalConfig = false, perDisplayActive = false;
  QString session, planId;
  quint64 generation = 0, planRevision = 0;
  bool planComplete = false;
  QMap<QString, QString> identityReferences;
  QMap<QString, QRect> legacyDesktops; // actual DINF rectangles, not individual displays
  QByteArray encode() const;
  static std::optional<WorkspaceSnapshot> decode(const QByteArray &);
};

// One source of truth for the shared canvas, inspector, draft serialization and
// route preview. It performs no discovery, trust changes, OS display changes,
// Settings writes or core commands.
class DesktopWorkspace
{
public:
  DesktopWorkspace(DeviceSnapshots, const QByteArray &saved = {}, const QByteArray &appliedLayout = {});
  const DeviceSnapshots &devices() const
  {
    return m_devices;
  }
  const WorkspaceState &state() const
  {
    return m_state;
  }
  const QMap<QString, QString> &conflicts() const
  {
    return m_conflicts;
  }
  const QString &error() const
  {
    return m_error;
  }
  bool dirty() const
  {
    return m_state.encode() != m_initial.encode();
  }
  bool hasUnappliedLayout() const
  {
    return m_state.layout.encode() != m_applied;
  }
  bool canUndo() const
  {
    return !m_history.isEmpty();
  }
  QVector<WorkspaceDisplay> displays() const;
  QStringList unknownDevices() const;
  Topologies topologies() const;
  QString validationError() const;
  std::optional<Layout> candidateForApply() const;
  QVector<Route> previewRoutes() const;
  bool moveDevice(const QString &, QPointF delta);
  bool moveDisplay(const QString &device, const QString &monitor, QPointF position, double snapThreshold = 0);
  bool setMode(ArrangementMode);
  bool calibratePhysicalSize(const QString &device, const QString &monitor, std::optional<QSizeF> mm);
  bool reviewOsLayout(const QString &device);
  void cancel();
  bool undo();
  static QString key(const QString &device, const QString &monitor);
  // A single scale is applied to the whole OS input desktop. Per-monitor Qt
  // logical origins cannot be mixed with independently scaled widths on Windows.
  static QMap<QString, QRectF> projectOsLayout(const Topology &);

private:
  const DeviceSnapshot *device(const QString &) const;
  void remember();
  void assignNumbers();
  void appendDevice(const DeviceSnapshot &, QPointF origin);
  DeviceSnapshots m_devices;
  WorkspaceState m_state, m_initial;
  struct History
  {
    WorkspaceState state;
    QMap<QString, QString> conflicts;
  };
  QVector<History> m_history;
  QMap<QString, QString> m_conflicts;
  QMap<QString, QString> m_initialConflicts;
  QByteArray m_applied;
  QString m_error;
};
} // namespace deskflow::display
