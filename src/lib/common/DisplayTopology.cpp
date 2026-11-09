// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DisplayTopology.h"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>
#include <cmath>

namespace deskflow::display {
namespace {
constexpr int maxBytes = 16384, maxMonitors = 16, maxPlacements = 16;
bool idValid(const QString &s)
{
  static const QRegularExpression re("^[a-zA-Z0-9_.-]{1,80}$");
  return re.match(s).hasMatch();
}
bool deviceValid(const QString &s)
{
  static const QRegularExpression re("^[a-zA-Z0-9_.-]{1,255}$");
  return re.match(s).hasMatch();
}
bool finite(double n)
{
  return std::isfinite(n) && std::abs(n) <= 10000000;
}
bool contains(const QRectF &r, QPointF p)
{
  return p.x() >= r.x() && p.x() < r.x() + r.width() && p.y() >= r.y() && p.y() < r.y() + r.height();
}
QJsonArray rect(const QRectF &r)
{
  return {r.x(), r.y(), r.width(), r.height()};
}
std::optional<QRectF> readRect(const QJsonValue &v)
{
  if (!v.isArray() || v.toArray().size() != 4)
    return {};
  const auto a = v.toArray();
  for (const auto &n : a)
    if (!n.isDouble() || !finite(n.toDouble()))
      return {};
  return QRectF(a[0].toDouble(), a[1].toDouble(), a[2].toDouble(), a[3].toDouble());
}
const Placement *placement(const Layout &l, const QString &device, const QString &monitor)
{
  for (const auto &p : l.placements)
    if (p.device == device && p.monitor == monitor)
      return &p;
  return nullptr;
}
const Monitor *monitor(const Topologies &t, const Placement &p)
{
  const auto it = t.constFind(p.device);
  return it == t.cend() ? nullptr : it->find(p.monitor);
}
QRectF canvas(const Placement &p, const Monitor &m)
{
  return {p.position, p.extent.value_or(m.logical.size())};
}
bool horizontal(Side side)
{
  return side == Side::Left || side == Side::Right;
}
bool adjacentAt(const QRectF &source, const QRectF &target, Side side, double axis)
{
  constexpr double epsilon = 0.01;
  const double a = side == Side::Left    ? source.x()
                   : side == Side::Right ? source.x() + source.width()
                   : side == Side::Top   ? source.y()
                                         : source.y() + source.height();
  const double b = side == Side::Left    ? target.x() + target.width()
                   : side == Side::Right ? target.x()
                   : side == Side::Top   ? target.y() + target.height()
                                         : target.y();
  const double start = horizontal(side) ? target.y() : target.x(),
               length = horizontal(side) ? target.height() : target.width();
  return std::abs(a - b) < epsilon && axis >= start && axis < start + length;
}
} // namespace
QString Topology::error() const
{
  if (monitors.isEmpty() || monitors.size() > maxMonitors)
    return "invalid-monitor-count";
  QSet<QString> ids;
  for (const auto &m : monitors) {
    if (!idValid(m.id) || ids.contains(m.id))
      return "invalid-monitor-id";
    ids.insert(m.id);
    const auto &r = m.logical;
    if (!finite(r.x()) || !finite(r.y()) || !finite(r.width()) || !finite(r.height()) || r.width() < 1 ||
        r.height() < 1 || r.width() > 32767 || r.height() > 32767)
      return "invalid-logical-geometry";
    if (m.input.width() < 1 || m.input.height() < 1 || m.input.x() < -32768 || m.input.y() < -32768 ||
        m.input.right() > 32767 || m.input.bottom() > 32767)
      return "unsupported-input-range";
    if (m.pixels.width() < 1 || m.pixels.height() < 1 || m.pixels.width() > 65536 || m.pixels.height() > 65536)
      return "invalid-pixel-resolution";
    const auto sx = m.input.width() / r.width(), sy = m.input.height() / r.height();
    if (sx < 0.25 || sx > 8 || sy < 0.25 || sy > 8)
      return "invalid-coordinate-scale";
    if (m.physicalMm &&
        (!finite(m.physicalMm->width()) || !finite(m.physicalMm->height()) || m.physicalMm->width() < 10 ||
         m.physicalMm->height() < 10 || m.physicalMm->width() > 3000 || m.physicalMm->height() > 3000))
      return "invalid-physical-size";
  }
  for (int i = 0; i < monitors.size(); ++i)
    for (int j = i + 1; j < monitors.size(); ++j)
      if (monitors[i].input.intersects(monitors[j].input))
        return "overlapping-input-monitors";
  return {};
}
QByteArray Topology::encode() const
{
  if (!error().isEmpty())
    return {};
  auto sorted = monitors;
  std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) { return a.id < b.id; });
  QJsonArray array;
  for (const auto &m : sorted) {
    QJsonObject o{
        {"id", m.id},
        {"logical", rect(m.logical)},
        {"input", rect(m.input)},
        {"pixels", QJsonArray{m.pixels.width(), m.pixels.height()}}
    };
    if (m.physicalMm)
      o["physicalMm"] = QJsonArray{m.physicalMm->width(), m.physicalMm->height()};
    array.append(o);
  }
  return QJsonDocument(QJsonObject{{"schema", 1}, {"monitors", array}}).toJson(QJsonDocument::Compact);
}
QByteArray Topology::revision() const
{
  return QCryptographicHash::hash(encode(), QCryptographicHash::Sha256).toHex();
}
std::optional<Topology> Topology::decode(const QByteArray &bytes)
{
  if (bytes.size() > maxBytes)
    return {};
  const auto doc = QJsonDocument::fromJson(bytes);
  if (!doc.isObject() || doc["schema"].toInt() != 1 || !doc["monitors"].isArray())
    return {};
  Topology result;
  const auto array = doc["monitors"].toArray();
  if (array.size() > maxMonitors)
    return {};
  for (const auto &v : array) {
    if (!v.isObject())
      return {};
    const auto o = v.toObject();
    const auto l = readRect(o["logical"]), n = readRect(o["input"]);
    const auto pixels = o["pixels"].toArray();
    if (!l || !n || pixels.size() != 2 || !o["id"].isString())
      return {};
    for (double d : {n->x(), n->y(), n->width(), n->height()})
      if (d != std::floor(d))
        return {};
    for (const auto &d : pixels)
      if (!d.isDouble() || !finite(d.toDouble()) || d.toDouble() != std::floor(d.toDouble()))
        return {};
    Monitor m{o["id"].toString(), *l, n->toRect(), QSize(pixels[0].toInt(), pixels[1].toInt()), {}};
    if (o.contains("physicalMm")) {
      const auto size = o["physicalMm"].toArray();
      if (size.size() != 2 || !size[0].isDouble() || !size[1].isDouble())
        return {};
      m.physicalMm = QSizeF(size[0].toDouble(), size[1].toDouble());
    }
    result.monitors.append(m);
  }
  if (!result.error().isEmpty())
    return {};
  return result;
}
const Monitor *Topology::find(const QString &id) const
{
  for (const auto &m : monitors)
    if (m.id == id)
      return &m;
  return nullptr;
}
QByteArray encodeTopologies(const Topologies &topologies)
{
  QJsonObject result;
  for (auto it = topologies.cbegin(); it != topologies.cend(); ++it)
    if (deviceValid(it.key()) && it->error().isEmpty())
      result[it.key()] = QJsonDocument::fromJson(it->encode()).object();
  return QJsonDocument(result).toJson(QJsonDocument::Compact);
}
Topologies decodeTopologies(const QByteArray &bytes)
{
  if (bytes.size() > 262144)
    return {};
  const auto object = QJsonDocument::fromJson(bytes).object();
  if (object.size() > 16)
    return {};
  Topologies result;
  for (auto it = object.begin(); it != object.end(); ++it) {
    const auto topology = Topology::decode(QJsonDocument(it.value().toObject()).toJson(QJsonDocument::Compact));
    if (deviceValid(it.key()) && topology)
      result[it.key()] = *topology;
  }
  return result;
}
QByteArray Layout::encode() const
{
  bool explicitExtents = false;
  QJsonArray array;
  for (const auto &p : placements) {
    QJsonObject item{{"device", p.device}, {"monitor", p.monitor}, {"x", p.position.x()}, {"y", p.position.y()}};
    if (p.extent) {
      item["extent"] = QJsonArray{p.extent->width(), p.extent->height()};
      explicitExtents = true;
    }
    array.append(item);
  }
  QJsonObject snapshots;
  for (auto it = revisions.cbegin(); it != revisions.cend(); ++it)
    snapshots[it.key()] = QString::fromLatin1(it.value());
  return QJsonDocument(
             QJsonObject{{"schema", explicitExtents ? 2 : 1}, {"placements", array}, {"revisions", snapshots}}
  ).toJson(QJsonDocument::Compact);
}
std::optional<Layout> Layout::decode(const QByteArray &bytes)
{
  if (bytes.size() > maxBytes)
    return {};
  const auto doc = QJsonDocument::fromJson(bytes);
  const auto schema = doc["schema"].toInt();
  if (!doc.isObject() || (schema != 1 && schema != 2) || !doc["placements"].isArray())
    return {};
  const auto a = doc["placements"].toArray();
  if (a.isEmpty() || a.size() > maxPlacements)
    return {};
  Layout result;
  QSet<QString> seen;
  for (const auto &v : a) {
    const auto p = v.toObject();
    const auto device = p["device"].toString(), id = p["monitor"].toString();
    if (!deviceValid(device) || !idValid(id) || !p["x"].isDouble() || !p["y"].isDouble() ||
        !finite(p["x"].toDouble()) || !finite(p["y"].toDouble()) || seen.contains(device + "/" + id))
      return {};
    std::optional<QSizeF> extent;
    if (p.contains("extent")) {
      if (schema != 2)
        return {};
      const auto size = p["extent"].toArray();
      if (size.size() != 2)
        return {};
      for (const auto &n : size)
        if (!n.isDouble() || !finite(n.toDouble()) || n.toDouble() < 1 || n.toDouble() > 1000000)
          return {};
      extent = QSizeF(size[0].toDouble(), size[1].toDouble());
    }
    seen.insert(device + "/" + id);
    result.placements.append({device, id, {p["x"].toDouble(), p["y"].toDouble()}, extent});
  }
  if (doc.object().contains("revisions") && !doc["revisions"].isObject())
    return {};
  const auto snapshots = doc["revisions"].toObject();
  static const QRegularExpression digest("^[a-f0-9]{64}$");
  for (auto it = snapshots.begin(); it != snapshots.end(); ++it) {
    if (!deviceValid(it.key()) || !digest.match(it.value().toString()).hasMatch())
      return {};
    result.revisions[it.key()] = it.value().toString().toLatin1();
  }
  return result;
}
QRectF Layout::canvasRect(const Topologies &topologies, const Placement &p) const
{
  const auto *m = monitor(topologies, p);
  return m ? canvas(p, *m) : QRectF{};
}
QString Layout::geometryError(const Topologies &topologies) const
{
  if (!decode(encode()))
    return "invalid-monitor-layout";
  for (const auto &p : placements) {
    const auto it = topologies.constFind(p.device);
    if (it == topologies.cend() || !it->error().isEmpty() || !it->find(p.monitor))
      return "monitor-unavailable";
  }
  QMap<QString, int> counts;
  for (const auto &p : placements)
    ++counts[p.device];
  for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
    if (it.value() != topologies[it.key()].monitors.size())
      return "monitor-set-changed";
    if (revisions.contains(it.key()) && revisions[it.key()] != topologies[it.key()].revision())
      return "monitor-geometry-changed";
  }
  for (int i = 0; i < placements.size(); ++i)
    for (int j = i + 1; j < placements.size(); ++j) {
      const auto a = canvas(placements[i], *monitor(topologies, placements[i])),
                 b = canvas(placements[j], *monitor(topologies, placements[j]));
      if (a.intersected(b).width() > 0.01 && a.intersected(b).height() > 0.01)
        return "monitor-layout-overlap";
    }
  return {};
}
Layout Layout::available(const Topologies &topologies) const
{
  Layout result;
  QSet<QString> valid;
  for (auto it = topologies.cbegin(); it != topologies.cend(); ++it) {
    Layout device;
    for (const auto &p : placements)
      if (p.device == it.key())
        device.placements.append(p);
    if (revisions.contains(it.key()))
      device.revisions[it.key()] = revisions[it.key()];
    if (device.geometryError(topologies).isEmpty()) {
      valid.insert(it.key());
      if (device.revisions.contains(it.key()))
        result.revisions[it.key()] = device.revisions[it.key()];
    }
  }
  for (const auto &p : placements)
    if (valid.contains(p.device))
      result.placements.append(p);
  // Never hide overlap/corruption by turning it into an arbitrary route.
  if (!result.geometryError(topologies).isEmpty())
    return {};
  return result;
}
QString Layout::error(const Topologies &topologies) const
{
  const auto geometry = geometryError(topologies);
  if (!geometry.isEmpty())
    return geometry;
  QSet<QString> visited;
  const auto key = [](const QString &d, const QString &m) { return d + "/" + m; };
  visited.insert(key(placements.front().device, placements.front().monitor));
  const auto links = routes(topologies);
  bool changed = true;
  while (changed) {
    changed = false;
    for (const auto &r : links)
      if (visited.contains(key(r.device, r.monitor)) && !visited.contains(key(r.targetDevice, r.targetMonitor))) {
        visited.insert(key(r.targetDevice, r.targetMonitor));
        changed = true;
      }
    for (const auto &p : placements)
      if (visited.contains(key(p.device, p.monitor))) {
        const auto *m = monitor(topologies, p);
        const QRectF a = m->input;
        for (const auto &q : placements)
          if (q.device == p.device && !visited.contains(key(q.device, q.monitor))) {
            const QRectF b = monitor(topologies, q)->input;
            for (auto side : {Side::Left, Side::Right, Side::Top, Side::Bottom}) {
              const bool h = horizontal(side);
              const double low = std::max(h ? a.y() : a.x(), h ? b.y() : b.x()),
                           high = std::min(
                               h ? a.y() + a.height() : a.x() + a.width(), h ? b.y() + b.height() : b.x() + b.width()
                           );
              if (high > low && adjacentAt(a, b, side, (low + high) / 2)) {
                visited.insert(key(q.device, q.monitor));
                changed = true;
                break;
              }
            }
          }
      }
  }
  if (visited.size() != placements.size())
    return "no-reachable-display-edge";
  return {};
}
std::optional<QString> Layout::monitorAt(const Topologies &t, const QString &device, QPointF input) const
{
  const auto it = t.constFind(device);
  if (it == t.cend())
    return {};
  for (const auto &m : it->monitors)
    if (contains(m.input, input))
      return m.id;
  return {};
}
std::optional<Side> Layout::leaving(
    const Topologies &t, const QString &device, const QString &id, QPointF previous, QPointF current, int zone
) const
{
  const auto p = placement(*this, device, id);
  const auto m = p ? monitor(t, *p) : nullptr;
  if (!m)
    return {};
  const auto r = m->input;
  const auto delta = current - previous;
  std::optional<Side> side;
  if ((delta.x() < 0 || (zone > 0 && delta.x() == 0)) && current.x() < r.x() + zone)
    side = Side::Left;
  else if ((delta.x() > 0 || (zone > 0 && delta.x() == 0)) && current.x() >= r.x() + r.width() - zone)
    side = Side::Right;
  else if ((delta.y() < 0 || (zone > 0 && delta.y() == 0)) && current.y() < r.y() + zone)
    side = Side::Top;
  else if ((delta.y() > 0 || (zone > 0 && delta.y() == 0)) && current.y() >= r.y() + r.height() - zone)
    side = Side::Bottom;
  if (!side)
    return {};
  const double axis = horizontal(*side) ? std::clamp(current.y(), double(r.y()) + 0.5, double(r.y() + r.height()) - 0.5)
                                        : std::clamp(current.x(), double(r.x()) + 0.5, double(r.x() + r.width()) - 0.5);
  for (const auto &other : t[device].monitors)
    if (other.id != id && adjacentAt(r, other.input, *side, axis))
      return {};
  return side;
}
QVector<Route> Layout::routes(const Topologies &t) const
{
  QVector<Route> result;
  // Cut OS-owned intervals out of a canvas contact. This same list drives the
  // green edge preview, connectivity validation and input coordinate mapping.
  const auto cut = [](QVector<QPair<double, double>> intervals, double low, double high) {
    QVector<QPair<double, double>> next;
    for (const auto &[a, b] : intervals) {
      if (high <= a || low >= b)
        next.append({a, b});
      else {
        if (low > a)
          next.append({a, low});
        if (high < b)
          next.append({high, b});
      }
    }
    return next;
  };
  const auto removeNative = [&](QVector<QPair<double, double>> intervals, const Placement &p, const Monitor &m,
                                Side side) {
    const bool h = horizontal(side);
    const QRectF r = m.input;
    const auto c = canvas(p, m);
    for (const auto &other : t[p.device].monitors) {
      if (other.id == m.id)
        continue;
      const QRectF o = other.input;
      const double low = std::max(h ? r.y() : r.x(), h ? o.y() : o.x());
      const double high =
          std::min(h ? r.y() + r.height() : r.x() + r.width(), h ? o.y() + o.height() : o.x() + o.width());
      if (high <= low || !adjacentAt(r, o, side, (low + high) / 2))
        continue;
      const double origin = h ? r.y() : r.x(), size = h ? r.height() : r.width(), canvasOrigin = h ? c.y() : c.x(),
                   canvasSize = h ? c.height() : c.width();
      intervals =
          cut(intervals, canvasOrigin + (low - origin) / size * canvasSize,
              canvasOrigin + (high - origin) / size * canvasSize);
    }
    return intervals;
  };
  for (const auto &p : placements) {
    const auto *m = monitor(t, p);
    if (!m)
      return {};
    const auto a = canvas(p, *m);
    for (const auto &q : placements) {
      if (q.device == p.device)
        continue;
      const auto *n = monitor(t, q);
      if (!n)
        return {};
      const auto b = canvas(q, *n);
      for (auto side : {Side::Left, Side::Right, Side::Top, Side::Bottom}) {
        const bool h = horizontal(side);
        const double low = std::max(h ? a.y() : a.x(), h ? b.y() : b.x());
        const double high =
            std::min(h ? a.y() + a.height() : a.x() + a.width(), h ? b.y() + b.height() : b.x() + b.width());
        if (high - low < 0.01 || !adjacentAt(a, b, side, (low + high) / 2))
          continue;
        const auto reverse = side == Side::Left    ? Side::Right
                             : side == Side::Right ? Side::Left
                             : side == Side::Top   ? Side::Bottom
                                                   : Side::Top;
        auto intervals = removeNative({{low, high}}, p, *m, side);
        intervals = removeNative(intervals, q, *n, reverse);
        const double ao = h ? a.y() : a.x(), as = h ? a.height() : a.width(), bo = h ? b.y() : b.x(),
                     bs = h ? b.height() : b.width();
        const double edge = side == Side::Left    ? a.x()
                            : side == Side::Right ? a.x() + a.width()
                            : side == Side::Top   ? a.y()
                                                  : a.y() + a.height();
        for (const auto &[lo, hi] : intervals)
          if (hi - lo >= 0.01)
            result.append(
                {p.device, p.monitor, q.device, q.monitor, side, (lo - ao) / as, (hi - ao) / as, (lo - bo) / bs,
                 (hi - bo) / bs, h ? QLineF(edge, lo, edge, hi) : QLineF(lo, edge, hi, edge)}
            );
      }
    }
  }
  return result;
}
std::optional<Crossing>
Layout::cross(const Topologies &t, const QString &device, const QString &id, Side side, QPointF input, int inset) const
{
  if (!error(t).isEmpty())
    return {};
  return crossValidated(t, routes(t), device, id, side, input, inset);
}
std::optional<Crossing> Layout::crossValidated(
    const Topologies &t, const QVector<Route> &links, const QString &device, const QString &id, Side side,
    QPointF input, int inset
) const
{
  const auto p = placement(*this, device, id);
  const auto m = p ? monitor(t, *p) : nullptr;
  if (!m)
    return {};
  const auto native = m->input;
  const bool h = horizontal(side);
  const double fraction =
      h ? (input.y() - native.y() + 0.5) / native.height() : (input.x() - native.x() + 0.5) / native.width();
  for (const auto &route : links) {
    if (route.device != device || route.monitor != id || route.side != side || fraction < route.sourceStart ||
        fraction >= route.sourceEnd)
      continue;
    const auto target = placement(*this, route.targetDevice, route.targetMonitor);
    const auto dest = monitor(t, *target);
    const auto r = dest->input;
    const double f = route.targetStart + (fraction - route.sourceStart) / (route.sourceEnd - route.sourceStart) *
                                             (route.targetEnd - route.targetStart);
    const int ix = std::clamp(inset, 0, qMax(0, (r.width() - 1) / 2)),
              iy = std::clamp(inset, 0, qMax(0, (r.height() - 1) / 2));
    const QPoint result =
        h ? QPoint(side == Side::Right ? r.x() + ix : r.right() - ix, r.y() + int(std::floor(f * r.height())))
          : QPoint(r.x() + int(std::floor(f * r.width())), side == Side::Bottom ? r.y() + iy : r.bottom() - iy);
    return Crossing{route.targetDevice, route.targetMonitor, result, side};
  }
  return {};
}
QPoint Layout::clamp(const Topologies &t, const QString &device, const QString &id, QPointF input) const
{
  const auto p = placement(*this, device, id);
  const auto m = p ? monitor(t, *p) : nullptr;
  if (!m)
    return input.toPoint();
  const auto r = m->input;
  return {
      int(std::clamp(input.x(), double(r.x()), double(r.right()))),
      int(std::clamp(input.y(), double(r.y()), double(r.bottom())))
  };
}
} // namespace deskflow::display
