// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "experience/LayoutModel.h"
#include <QStringList>

namespace deskflow::gui {
// Versioned, bounded metadata for the existing core's whole-computer links.
// It contains no identities, permissions, input, monitor IDs or trust records.
struct DesktopLayout
{
  static QString displayNameError(const QString &name);
  static QString draftError(const std::vector<experience::Desktop> &desktops);
  static QString errorText(const std::string &error);
  std::vector<experience::Desktop> desktops;
  QString error;
  static DesktopLayout decode(const QString &json);
  static QString encode(const std::vector<experience::Desktop> &desktops);
  static QString validate(const std::vector<experience::Desktop> &desktops, const QStringList &names);
};
} // namespace deskflow::gui
