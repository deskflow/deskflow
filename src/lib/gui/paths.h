/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Settings.h"

#include <QDir>

namespace deskflow::gui::paths {

/**
 * @brief Gets the org config dir (parent of app config dir).
 */
inline QDir configDir(const bool persist = false)
{
  const QDir configDir(Settings::settingsPath());

  // HACK: since we have the org name set to the app name, the config dir is
  // confusing. make this simple by using the org dir instead.
  // use `filePath("..")` instead of `cdUp` to avoid the existence check.
  const QDir orgDir = configDir.filePath("..");

  if (persist) {
    const auto orgDirPath = orgDir.absolutePath();
    if (!QDir().mkpath(orgDirPath)) {
      qFatal("failed to persist config dir: %s", qPrintable(orgDirPath));
    }
  }

  return orgDir.absolutePath();
}

} // namespace deskflow::gui::paths
