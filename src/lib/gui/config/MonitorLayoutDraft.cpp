// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "MonitorLayoutDraft.h"
#include "DesktopLayout.h"
#include <QCryptographicHash>
#include <QObject>
#include <algorithm>

namespace deskflow::gui {
QString MonitorLayoutDraft::token(const display::Placement &p)
{
  return "display-" +
         QString::fromLatin1(
             QCryptographicHash::hash((p.device + "/" + p.monitor).toUtf8(), QCryptographicHash::Sha256).toHex()
         );
}
MonitorLayoutDraft::MonitorLayoutDraft(
    display::Topologies snapshots, const QStringList &devices, const QByteArray &saved
)
    : topologies(std::move(snapshots))
{
  for (const auto &device : devices)
    if (!topologies.contains(device) || !topologies[device].error().isEmpty()) {
      error = QObject::tr(
          "Connect every computer using a version that shares individual displays. Compatibility layout remains "
          "available."
      );
      return;
    }
  if (!saved.isEmpty()) {
    const auto decoded = display::Layout::decode(saved);
    if (!decoded) {
      error = QObject::tr("The saved display layout cannot be read. It has been preserved.");
      return;
    }
    layout = *decoded;
    if (!layout.error(topologies).isEmpty()) {
      error = QObject::tr(
          "Displays or resolution changed. Review a new display arrangement before applying it. The saved layout is "
          "preserved."
      );
      return;
    }
    QStringList placed;
    for (const auto &p : layout.placements)
      if (!placed.contains(p.device))
        placed.append(p.device);
    auto expected = devices;
    placed.sort();
    expected.sort();
    if (placed != expected) {
      error = QObject::tr("The computer list changed. Review a new display arrangement before applying it.");
      return;
    }
  } else {
    double x = 0;
    for (const auto &device : devices) {
      auto monitors = topologies[device].monitors;
      std::sort(monitors.begin(), monitors.end(), [](const auto &a, const auto &b) {
        return a.input.x() != b.input.x() ? a.input.x() < b.input.x() : a.input.y() < b.input.y();
      });
      for (const auto &m : monitors) {
        layout.placements.append({device, m.id, {x, 0}});
        x += m.logical.width();
      }
      layout.revisions[device] = topologies[device].revision();
    }
    if (!layout.error(topologies).isEmpty())
      error = QObject::tr("This display topology cannot be represented safely. Use the compatibility layout.");
  }
}
std::vector<experience::Desktop> MonitorLayoutDraft::desktops() const
{
  std::vector<experience::Desktop> result;
  if (!error.isEmpty())
    return result;
  QMap<QString, int> indices;
  for (const auto &p : layout.placements) {
    const auto m = topologies.constFind(p.device)->find(p.monitor);
    if (!m)
      return {};
    const auto label = QObject::tr("%1 · Display %2").arg(p.device.left(40)).arg(++indices[p.device]);
    result.push_back(
        {token(p).toStdString(), p.position.x(), p.position.y(), m->logical.width(), m->logical.height(),
         label.toStdString()}
    );
  }
  return result;
}
bool MonitorLayoutDraft::apply(const std::vector<experience::Desktop> &draft)
{
  validationError = DesktopLayout::draftError(draft);
  if (!error.isEmpty() || draft.size() != layout.placements.size() || !validationError.isEmpty())
    return false;
  auto candidate = layout;
  for (auto &p : candidate.placements) {
    const auto key = token(p).toStdString();
    const auto found = std::find_if(draft.begin(), draft.end(), [&](const auto &d) { return d.name == key; });
    const auto m = topologies[p.device].find(p.monitor);
    if (found == draft.end() || !m || found->width != m->logical.width() || found->height != m->logical.height())
      return false;
    p.position = {found->x, found->y};
  }
  if (!candidate.error(topologies).isEmpty()) {
    validationError = QObject::tr(
        "Every display needs a reachable edge. Keep OS-owned local edges free and join exposed edges between computers."
    );
    return false;
  }
  layout = std::move(candidate);
  return true;
}
} // namespace deskflow::gui
