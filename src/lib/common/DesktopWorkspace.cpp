// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DesktopWorkspace.h"
#include "PeerAuthentication.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>
#include <cmath>

namespace deskflow::display {
namespace {
bool validDevice(const QString &id)
{
  static const QRegularExpression pattern("\\A[a-zA-Z0-9_.-]{1,255}\\z");
  return pattern.match(id).hasMatch();
}
bool validKey(const QString &key)
{
  static const QRegularExpression pattern("\\A[a-zA-Z0-9_.-]{1,255}/[a-zA-Z0-9_.-]{1,80}\\z");
  return pattern.match(key).hasMatch();
}
bool physicalSize(QSizeF size)
{
  return std::isfinite(size.width()) && std::isfinite(size.height()) && size.width() >= 10 && size.height() >= 10 &&
         size.width() <= 3000 && size.height() <= 3000;
}
bool coordinate(QPointF point)
{
  return std::isfinite(point.x()) && std::isfinite(point.y()) && std::abs(point.x()) <= 10000000 &&
         std::abs(point.y()) <= 10000000;
}
} // namespace

QString DesktopWorkspace::key(const QString &device, const QString &monitor)
{
  return device + '/' + monitor;
}

QByteArray WorkspaceState::encode() const
{
  QJsonObject numbers, calibration, identities;
  for (auto it = identityReferences.cbegin(); it != identityReferences.cend(); ++it)
    identities[it.key()] = it.value();
  for (auto it = displayNumbers.cbegin(); it != displayNumbers.cend(); ++it)
    numbers[it.key()] = it.value();
  for (auto it = physicalCalibration.cbegin(); it != physicalCalibration.cend(); ++it)
    calibration[it.key()] = QJsonArray{it->width(), it->height()};
  return QJsonDocument(
             QJsonObject{
                 {"schema", 1},
                 {"mode", mode == ArrangementMode::PreserveOsGroups ? "os-groups" : "custom-screen-routes"},
                 {"layout", QJsonDocument::fromJson(layout.encode()).object()},
                 {"numbers", numbers},
                 {"physicalCalibrationMm", calibration},
                 {"revision", QString::number(revision)},
                 {"identityReferences", identities}
             }
  ).toJson(QJsonDocument::Compact);
}

std::optional<WorkspaceState> WorkspaceState::decode(const QByteArray &bytes)
{
  if (bytes.size() > 262144)
    return {};
  const auto doc = QJsonDocument::fromJson(bytes);
  if (!doc.isObject() || doc["schema"].toInt() != 1 || !doc["layout"].isObject() || !doc["numbers"].isObject() ||
      !doc["physicalCalibrationMm"].isObject())
    return {};
  WorkspaceState state;
  if (doc.object().contains("revision")) {
    const auto value = doc["revision"].toString();
    bool ok = false;
    state.revision = value.toULongLong(&ok);
    if (!ok || QString::number(state.revision) != value)
      return {};
  }
  if (doc.object().contains("identityReferences")) {
    if (!doc["identityReferences"].isObject() || doc["identityReferences"].toObject().size() > 16)
      return {};
    const auto identities = doc["identityReferences"].toObject();
    for (auto it = identities.begin(); it != identities.end(); ++it) {
      if (!validDevice(it.key()) || auth::fingerprint(it->toString()) != it->toString() || it->toString().isEmpty())
        return {};
      state.identityReferences[it.key()] = it->toString();
    }
  }
  const auto mode = doc["mode"].toString();
  if (mode != "os-groups" && mode != "custom-screen-routes")
    return {};
  state.mode = mode == "os-groups" ? ArrangementMode::PreserveOsGroups : ArrangementMode::CustomScreenRoutes;
  const auto layout = doc["layout"].toObject();
  if (!layout["placements"].isArray())
    return {};
  if (!layout["placements"].toArray().isEmpty()) {
    const auto decoded = Layout::decode(QJsonDocument(layout).toJson(QJsonDocument::Compact));
    if (!decoded)
      return {};
    state.layout = *decoded;
  } else if (layout["schema"].toInt() != 1 || !layout["revisions"].toObject().isEmpty())
    return {};
  const auto numbers = doc["numbers"].toObject(), calibration = doc["physicalCalibrationMm"].toObject();
  if (numbers.size() > 256 || calibration.size() > 256)
    return {};
  QMap<QString, QSet<int>> used;
  for (auto it = numbers.begin(); it != numbers.end(); ++it) {
    const auto n = it.value().toDouble(-1);
    const auto device = it.key().section('/', 0, 0);
    if (!validKey(it.key()) || !it->isDouble() || !std::isfinite(n) || n != std::floor(n) || n < 1 || n > 65535 ||
        used[device].contains(int(n)))
      return {};
    state.displayNumbers[it.key()] = int(n);
    used[device].insert(int(n));
  }
  for (auto it = calibration.begin(); it != calibration.end(); ++it) {
    const auto size = it->toArray();
    if (!validKey(it.key()) || size.size() != 2 || !size[0].isDouble() || !size[1].isDouble())
      return {};
    const QSizeF mm(size[0].toDouble(), size[1].toDouble());
    if (!physicalSize(mm))
      return {};
    state.physicalCalibration[it.key()] = mm;
  }
  return state;
}

QByteArray WorkspaceSnapshot::encode() const
{
  QJsonObject identities, legacy;
  for (auto it = identityReferences.cbegin(); it != identityReferences.cend(); ++it)
    identities[it.key()] = it.value();
  for (auto it = legacyDesktops.cbegin(); it != legacyDesktops.cend(); ++it)
    legacy[it.key()] = QJsonArray{it->x(), it->y(), it->width(), it->height()};
  const auto bytes = QJsonDocument(
                         QJsonObject{
                             {"schema", 1},
                             {"devices", QJsonArray::fromStringList(devices)},
                             {"online", QJsonArray::fromStringList(onlineDevices)},
                             {"topologyCapable", QJsonArray::fromStringList(topologyCapable)},
                             {"topologies", QJsonDocument::fromJson(encodeTopologies(topologies)).object()},
                             {"applied", QJsonDocument::fromJson(applied.encode()).object()},
                             {"externalConfig", externalConfig},
                             {"perDisplayActive", perDisplayActive},
                             {"session", session},
                             {"generation", QString::number(generation)},
                             {"planRevision", QString::number(planRevision)},
                             {"planId", planId},
                             {"planComplete", planComplete},
                             {"identityReferences", identities},
                             {"legacyDesktops", legacy}
                         }
  ).toJson(QJsonDocument::Compact);
  // Leave room for the server's ordinary topology in the bounded DTOP frame.
  return bytes.size() <= 12000 ? bytes : QByteArray{};
}
std::optional<WorkspaceSnapshot> WorkspaceSnapshot::decode(const QByteArray &bytes)
{
  if (bytes.isEmpty() || bytes.size() > 12000)
    return {};
  const auto doc = QJsonDocument::fromJson(bytes);
  if (!doc.isObject() || doc["schema"].toInt() != 1 || !doc["devices"].isArray() || !doc["online"].isArray() ||
      !doc["topologyCapable"].isArray() || !doc["topologies"].isObject() || !doc["applied"].isObject() ||
      !doc["externalConfig"].isBool() || !doc["perDisplayActive"].isBool())
    return {};
  WorkspaceSnapshot result;
  const auto list = [](const QJsonArray &array, QStringList &out) {
    if (array.size() > 16)
      return false;
    for (const auto &v : array) {
      if (!v.isString() || !validDevice(v.toString()) || out.contains(v.toString()))
        return false;
      out.append(v.toString());
    }
    return true;
  };
  if (!list(doc["devices"].toArray(), result.devices) || !list(doc["online"].toArray(), result.onlineDevices) ||
      !list(doc["topologyCapable"].toArray(), result.topologyCapable))
    return {};
  for (const auto &id : result.onlineDevices)
    if (!result.devices.contains(id))
      return {};
  for (const auto &id : result.topologyCapable)
    if (!result.devices.contains(id))
      return {};
  const auto source = doc["topologies"].toObject();
  result.topologies = decodeTopologies(QJsonDocument(source).toJson(QJsonDocument::Compact));
  if (result.topologies.size() != source.size())
    return {};
  for (auto it = result.topologies.cbegin(); it != result.topologies.cend(); ++it)
    if (!result.devices.contains(it.key()))
      return {};
  const auto applied = WorkspaceState::decode(QJsonDocument(doc["applied"].toObject()).toJson(QJsonDocument::Compact));
  if (!applied)
    return {};
  result.applied = *applied;
  for (const auto &p : result.applied.layout.placements)
    if (!result.devices.contains(p.device))
      return {};
  for (const auto &k : result.applied.displayNumbers.keys())
    if (!result.devices.contains(k.section('/', 0, 0)))
      return {};
  for (const auto &k : result.applied.physicalCalibration.keys())
    if (!result.devices.contains(k.section('/', 0, 0)))
      return {};
  result.externalConfig = doc["externalConfig"].toBool();
  result.perDisplayActive = doc["perDisplayActive"].toBool();
  if (doc.object().contains("session")) {
    result.session = doc["session"].toString();
    result.planId = doc["planId"].toString();
    if (!result.session.isEmpty() && !auth::identifier(result.session))
      return {};
    if (!result.planId.isEmpty() && auth::fingerprint(result.planId) != result.planId)
      return {};
    const auto counter = [](const QJsonValue &value, quint64 &out) {
      bool ok = false;
      out = value.toString().toULongLong(&ok);
      return ok && QString::number(out) == value.toString();
    };
    if (!counter(doc["generation"], result.generation) || !counter(doc["planRevision"], result.planRevision) ||
        !doc["planComplete"].isBool())
      return {};
    result.planComplete = doc["planComplete"].toBool();
  }
  if (doc.object().contains("identityReferences")) {
    if (!doc["identityReferences"].isObject())
      return {};
    const auto identities = doc["identityReferences"].toObject();
    for (auto it = identities.begin(); it != identities.end(); ++it) {
      if (!result.onlineDevices.contains(it.key()) || it->toString().isEmpty() ||
          auth::fingerprint(it->toString()) != it->toString())
        return {};
      result.identityReferences[it.key()] = it->toString();
    }
  }
  if (doc.object().contains("legacyDesktops")) {
    if (!doc["legacyDesktops"].isObject())
      return {};
    const auto legacy = doc["legacyDesktops"].toObject();
    for (auto it = legacy.begin(); it != legacy.end(); ++it) {
      if (!result.onlineDevices.contains(it.key()) || result.topologyCapable.contains(it.key()))
        return {};
      const auto rect = it->toArray();
      if (rect.size() != 4)
        return {};
      for (const auto &v : rect)
        if (!v.isDouble() || v.toDouble() != v.toInt() || qAbs(v.toInt()) > 1000000)
          return {};
      if (rect[2].toInt() <= 0 || rect[3].toInt() <= 0)
        return {};
      result.legacyDesktops[it.key()] = {rect[0].toInt(), rect[1].toInt(), rect[2].toInt(), rect[3].toInt()};
    }
  }
  return result;
}

QMap<QString, QRectF> DesktopWorkspace::projectOsLayout(const Topology &topology)
{
  QMap<QString, QRectF> result;
  if (!topology.error().isEmpty())
    return result;
  const auto reference =
      std::min_element(topology.monitors.cbegin(), topology.monitors.cend(), [](const auto &a, const auto &b) {
        return a.id < b.id;
      });
  const double scale = reference->input.width() / reference->logical.width();
  QRectF bounds;
  for (const auto &monitor : topology.monitors)
    bounds = bounds.united(monitor.input);
  for (const auto &monitor : topology.monitors) {
    const QPointF position = (QPointF(monitor.input.topLeft()) - bounds.topLeft()) / scale;
    result[monitor.id] = {position, QSizeF(monitor.input.size()) / scale};
  }
  return result;
}

DesktopWorkspace::DesktopWorkspace(DeviceSnapshots devices, const QByteArray &saved, const QByteArray &applied)
    : m_devices(std::move(devices)),
      m_applied(applied)
{
  QSet<QString> seen;
  if (m_devices.size() > 16) {
    m_error = "too-many-devices";
    return;
  }
  for (const auto &d : m_devices) {
    if (!validDevice(d.id) || seen.contains(d.id)) {
      m_error = "invalid-device-list";
      return;
    }
    seen.insert(d.id);
  }
  if (!saved.isEmpty()) {
    const auto parsed = WorkspaceState::decode(saved);
    if (!parsed) {
      m_error = "unreadable-workspace-preserved";
      return;
    }
    m_state = *parsed;
  } else if (!applied.isEmpty()) {
    const auto parsed = Layout::decode(applied);
    if (!parsed) {
      m_error = "unreadable-applied-layout-preserved";
      return;
    }
    m_state.layout = *parsed;
    // An old individually positioned arrangement is never silently regrouped.
    m_state.mode = ArrangementMode::CustomScreenRoutes;
  }
  // A protocol rename may reuse positions only through a unique current TLS
  // identity binding. Names, cached metadata and duplicate certificates cannot
  // silently claim another device's arrangement.
  for (const auto &d : m_devices)
    if (!d.identityReference.isEmpty() && (d.local || d.authenticated)) {
      if (auth::fingerprint(d.identityReference) != d.identityReference)
        continue;
      QString old;
      int matches = 0, currentMatches = 0;
      for (const auto &peer : m_devices)
        if (peer.identityReference == d.identityReference)
          ++currentMatches;
      for (auto it = m_state.identityReferences.cbegin(); it != m_state.identityReferences.cend(); ++it)
        if (it.value() == d.identityReference) {
          old = it.key();
          ++matches;
        }
      if (matches == 1 && currentMatches == 1 && old != d.id && !seen.contains(old) &&
          !m_state.identityReferences.contains(d.id)) {
        for (auto &p : m_state.layout.placements)
          if (p.device == old)
            p.device = d.id;
        if (m_state.layout.revisions.contains(old))
          m_state.layout.revisions[d.id] = m_state.layout.revisions.take(old);
        const auto renameKeys = [&](auto &map) {
          for (const auto &key : map.keys())
            if (key.startsWith(old + '/'))
              map[d.id + key.mid(old.size())] = map.take(key);
        };
        renameKeys(m_state.displayNumbers);
        renameKeys(m_state.physicalCalibration);
        m_state.identityReferences.remove(old);
      }
      if (m_state.identityReferences.contains(d.id) && m_state.identityReferences[d.id] != d.identityReference)
        m_conflicts[d.id] = "device-identity-changed";
      else
        m_state.identityReferences[d.id] = d.identityReference;
    }
  for (const auto &p : m_state.layout.placements)
    if (!seen.contains(p.device))
      m_conflicts[p.device] = "device-removed";
  const auto known = topologies();
  double nextX = 0;
  for (const auto &p : m_state.layout.placements) {
    const auto rect = m_state.layout.canvasRect(known, p);
    if (!rect.isEmpty())
      nextX = std::max(nextX, rect.right());
  }
  for (const auto &d : m_devices) {
    if (!d.topology || !d.topology->error().isEmpty())
      continue;
    QVector<QString> savedIds, currentIds;
    bool present = false;
    for (const auto &p : m_state.layout.placements)
      if (p.device == d.id) {
        present = true;
        savedIds.append(p.monitor);
      }
    for (const auto &m : d.topology->monitors)
      currentIds.append(m.id);
    if (present) {
      std::sort(savedIds.begin(), savedIds.end());
      std::sort(currentIds.begin(), currentIds.end());
      if (!m_conflicts.contains(d.id) && savedIds != currentIds)
        m_conflicts[d.id] = "monitor-set-changed";
      else if (!m_conflicts.contains(d.id) && m_state.layout.revisions.contains(d.id) &&
               m_state.layout.revisions[d.id] != d.topology->revision())
        m_conflicts[d.id] = "monitor-geometry-changed";
    } else {
      appendDevice(d, {nextX, 0});
      for (const auto &p : m_state.layout.placements)
        if (p.device == d.id)
          nextX = std::max(nextX, m_state.layout.canvasRect(known, p).right());
    }
  }
  assignNumbers();
  m_initial = m_state;
  m_initialConflicts = m_conflicts;
}

const DeviceSnapshot *DesktopWorkspace::device(const QString &id) const
{
  for (const auto &d : m_devices)
    if (d.id == id)
      return &d;
  return nullptr;
}
Topologies DesktopWorkspace::topologies() const
{
  Topologies result;
  for (const auto &d : m_devices)
    if (d.topology && d.topology->error().isEmpty())
      result[d.id] = *d.topology;
  return result;
}
void DesktopWorkspace::appendDevice(const DeviceSnapshot &d, QPointF origin)
{
  if (!d.topology)
    return;
  const auto projected = projectOsLayout(*d.topology);
  for (auto it = projected.cbegin(); it != projected.cend(); ++it)
    m_state.layout.placements.append({d.id, it.key(), origin + it->topLeft(), it->size()});
  m_state.layout.revisions[d.id] = d.topology->revision();
}
void DesktopWorkspace::assignNumbers()
{
  for (const auto &d : m_devices) {
    if (!d.topology)
      continue;
    int next = 1;
    for (auto it = m_state.displayNumbers.cbegin(); it != m_state.displayNumbers.cend(); ++it)
      if (it.key().startsWith(d.id + '/'))
        next = std::max(next, it.value() + 1);
    auto monitors = d.topology->monitors;
    std::sort(monitors.begin(), monitors.end(), [](const auto &a, const auto &b) {
      if (a.input.y() != b.input.y())
        return a.input.y() < b.input.y();
      return a.input.x() != b.input.x() ? a.input.x() < b.input.x() : a.id < b.id;
    });
    for (const auto &m : monitors)
      if (!m_state.displayNumbers.contains(key(d.id, m.id))) {
        if (next > 65535) {
          m_error = "screen-number-limit";
          return;
        }
        m_state.displayNumbers[key(d.id, m.id)] = next++;
      }
  }
}
QVector<WorkspaceDisplay> DesktopWorkspace::displays() const
{
  QVector<WorkspaceDisplay> result;
  const auto known = topologies();
  for (const auto &p : m_state.layout.placements) {
    const auto *d = device(p.device);
    if (!d || !d->topology)
      continue;
    const auto *m = d->topology->find(p.monitor);
    if (!m)
      continue;
    const auto k = key(p.device, p.monitor);
    const bool calibrated = m_state.physicalCalibration.contains(k);
    result.append(
        {p.device,
         d->displayName.isEmpty() ? d->id : d->displayName,
         p.monitor,
         m_state.displayNumbers.value(k),
         m->input,
         m->logical,
         m_state.layout.canvasRect(known, p),
         m->pixels,
         {m->input.width() / m->logical.width(), m->input.height() / m->logical.height()},
         {m->pixels.width() / m->logical.width(), m->pixels.height() / m->logical.height()},
         calibrated ? std::optional<QSizeF>(m_state.physicalCalibration[k]) : m->physicalMm,
         calibrated,
         d->observation == Observation::Cached || d->connection == Connection::Offline}
    );
  }
  return result;
}
QStringList DesktopWorkspace::unknownDevices() const
{
  QStringList result;
  for (const auto &d : m_devices)
    if (!d.topology || !d.topology->error().isEmpty())
      result.append(d.id);
  return result;
}
QString DesktopWorkspace::validationError() const
{
  if (!m_error.isEmpty())
    return m_error;
  if (!m_conflicts.isEmpty())
    return "topology-review-required";
  return m_state.layout.error(topologies());
}
std::optional<Layout> DesktopWorkspace::candidateForApply() const
{
  if (!validationError().isEmpty())
    return {};
  for (const auto &p : m_state.layout.placements) {
    const auto *d = device(p.device);
    if (!d || d->observation != Observation::Observed || d->routingCapability != Capability::Supported ||
        (!d->local && (d->connection != Connection::Live || !d->authenticated)))
      return {};
  }
  return m_state.layout;
}
QVector<Route> DesktopWorkspace::previewRoutes() const
{
  if (!validationError().isEmpty())
    return {};
  return m_state.layout.routes(topologies());
}
void DesktopWorkspace::remember()
{
  if (m_history.size() == 64)
    m_history.removeFirst();
  m_history.append({m_state, m_conflicts});
}
bool DesktopWorkspace::moveDevice(const QString &id, QPointF delta)
{
  if (!m_error.isEmpty() || !coordinate(delta))
    return false;
  auto next = m_state.layout;
  bool found = false;
  for (auto &p : next.placements)
    if (p.device == id) {
      p.position += delta;
      if (!coordinate(p.position))
        return false;
      found = true;
    }
  if (!found || delta.isNull())
    return false;
  remember();
  m_state.layout = std::move(next);
  return true;
}
bool DesktopWorkspace::moveDisplay(const QString &id, const QString &monitor, QPointF position, double snapThreshold)
{
  if (!m_error.isEmpty() || !coordinate(position) || !std::isfinite(snapThreshold) || snapThreshold < 0 ||
      snapThreshold > 1000)
    return false;
  for (auto &p : m_state.layout.placements)
    if (p.device == id && p.monitor == monitor) {
      if (snapThreshold > 0) {
        const auto known = topologies();
        const auto requested = position - p.position;
        const auto moving = [&](const Placement &item) {
          return item.device == id && (m_state.mode == ArrangementMode::PreserveOsGroups || item.monitor == monitor);
        };
        struct Correction
        {
          QPointF delta;
          QString sourceMonitor, targetDevice, targetMonitor;
        };
        QVector<Correction> corrections;
        for (const auto &a : m_state.layout.placements)
          if (moving(a)) {
            const auto r = m_state.layout.canvasRect(known, a).translated(requested);
            if (r.isEmpty())
              continue;
            for (const auto &b : m_state.layout.placements)
              if (b.device != id) {
                const auto s = m_state.layout.canvasRect(known, b);
                if (s.isEmpty())
                  continue;
                if (std::min(r.bottom(), s.bottom()) - std::max(r.top(), s.top()) > 0.01)
                  for (double dx : {s.left() - r.right(), s.right() - r.left()})
                    if (std::abs(dx) <= snapThreshold)
                      corrections.append({{dx, 0}, a.monitor, b.device, b.monitor});
                if (std::min(r.right(), s.right()) - std::max(r.left(), s.left()) > 0.01)
                  for (double dy : {s.top() - r.bottom(), s.bottom() - r.top()})
                    if (std::abs(dy) <= snapThreshold)
                      corrections.append({{0, dy}, a.monitor, b.device, b.monitor});
              }
          }
        std::sort(corrections.begin(), corrections.end(), [](const Correction &a, const Correction &b) {
          const double aa = std::abs(a.delta.x()) + std::abs(a.delta.y()),
                       bb = std::abs(b.delta.x()) + std::abs(b.delta.y());
          if (aa != bb)
            return aa < bb;
          if (a.delta != b.delta)
            return a.delta.x() != b.delta.x() ? a.delta.x() < b.delta.x() : a.delta.y() < b.delta.y();
          return a.sourceMonitor + '/' + a.targetDevice + '/' + a.targetMonitor <
                 b.sourceMonitor + '/' + b.targetDevice + '/' + b.targetMonitor;
        });
        for (const auto &correction : corrections) {
          auto candidate = m_state.layout;
          for (auto &item : candidate.placements)
            if (moving(item))
              item.position += requested + correction.delta;
          bool overlap = false;
          for (const auto &a : candidate.placements)
            if (moving(a))
              for (const auto &b : candidate.placements)
                if (!moving(b)) {
                  const auto intersect = candidate.canvasRect(known, a).intersected(candidate.canvasRect(known, b));
                  if (intersect.width() > 0.01 && intersect.height() > 0.01)
                    overlap = true;
                }
          if (overlap)
            continue;
          // Snap only to an actual exposed cross-computer route, not an edge
          // occupied by another screen of the same OS desktop.
          const auto routes = candidate.routes(known);
          if (std::any_of(routes.cbegin(), routes.cend(), [&](const Route &route) {
                return route.device == id && route.monitor == correction.sourceMonitor &&
                       route.targetDevice == correction.targetDevice && route.targetMonitor == correction.targetMonitor;
              })) {
            position += correction.delta;
            break;
          }
        }
      }
      if (!coordinate(position))
        return false;
      if (p.position == position)
        return false;
      if (m_state.mode == ArrangementMode::PreserveOsGroups)
        return moveDevice(id, position - p.position);
      remember();
      p.position = position;
      return true;
    }
  return false;
}
bool DesktopWorkspace::setMode(ArrangementMode mode)
{
  if (!m_error.isEmpty() || mode == m_state.mode)
    return false;
  // Returning to OS grouping requires an explicit review of each device. Do
  // not relabel an already-custom arrangement as an unchanged OS arrangement.
  if (mode == ArrangementMode::PreserveOsGroups) {
    for (const auto &d : m_devices) {
      if (!d.topology)
        continue;
      const auto projected = projectOsLayout(*d.topology);
      std::optional<QPointF> offset;
      for (const auto &p : m_state.layout.placements)
        if (p.device == d.id) {
          const auto *monitor = d.topology->find(p.monitor);
          if (!monitor || !projected.contains(p.monitor) ||
              p.extent.value_or(monitor->logical.size()) != projected[p.monitor].size())
            return false;
          const auto delta = p.position - projected[p.monitor].topLeft();
          if (offset && *offset != delta)
            return false;
          offset = delta;
        }
    }
  }
  remember();
  m_state.mode = mode;
  return true;
}
bool DesktopWorkspace::calibratePhysicalSize(const QString &id, const QString &monitor, std::optional<QSizeF> mm)
{
  const auto *d = device(id);
  if (!m_error.isEmpty() || !d || !d->topology || !d->topology->find(monitor) || (mm && !physicalSize(*mm)))
    return false;
  const auto k = key(id, monitor);
  if (mm && m_state.physicalCalibration.value(k) == *mm)
    return false;
  if (!mm && !m_state.physicalCalibration.contains(k))
    return false;
  remember();
  if (mm)
    m_state.physicalCalibration[k] = *mm;
  else
    m_state.physicalCalibration.remove(k);
  // Deliberately does not touch layout extent, position, revisions or routes.
  return true;
}
bool DesktopWorkspace::reviewOsLayout(const QString &id)
{
  if (m_conflicts.value(id) == "device-identity-changed")
    return false;
  const auto *d = device(id);
  if (!m_error.isEmpty() || !d || !d->topology || !d->topology->error().isEmpty())
    return false;
  QPointF origin;
  bool found = false;
  const auto known = topologies();
  QRectF bounds;
  for (const auto &p : m_state.layout.placements)
    if (p.device == id) {
      bounds = bounds.united(m_state.layout.canvasRect(known, p));
      found = true;
    }
  if (found)
    origin = bounds.topLeft();
  remember();
  auto &items = m_state.layout.placements;
  items.erase(std::remove_if(items.begin(), items.end(), [&](const auto &p) { return p.device == id; }), items.end());
  appendDevice(*d, origin);
  assignNumbers();
  m_conflicts.remove(id);
  return true;
}
void DesktopWorkspace::cancel()
{
  m_state = m_initial;
  m_conflicts = m_initialConflicts;
  m_history.clear();
}
bool DesktopWorkspace::undo()
{
  if (m_history.isEmpty())
    return false;
  const auto last = m_history.takeLast();
  m_state = last.state;
  m_conflicts = last.conflicts;
  return true;
}
} // namespace deskflow::display
