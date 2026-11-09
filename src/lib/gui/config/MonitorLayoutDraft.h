// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "common/DisplayTopology.h"
#include "experience/LayoutModel.h"
#include <QStringList>

namespace deskflow::gui {
// Adapts stable monitor identities to the existing draft canvas. The tokens are
// internal widget keys, never protocol computer names or user-entered data.
class MonitorLayoutDraft
{
public:
  MonitorLayoutDraft() = default;
  MonitorLayoutDraft(display::Topologies topologies, const QStringList &devices, const QByteArray &saved);
  QString error;
  QString validationError;
  display::Topologies topologies;
  display::Layout layout;
  std::vector<experience::Desktop> desktops() const;
  bool apply(const std::vector<experience::Desktop> &);
  static QString token(const display::Placement &);
};
} // namespace deskflow::gui
