/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "core/CoreTool.h"

#include <QDir>
#include <QStandardPaths>
#include <QString>

// TODO: Reduce duplication of these strings between here and SecureSocket.cpp
const auto kCertificateFilename = QStringLiteral("deskflow.pem");
const auto kSslDir = "tls";

namespace deskflow::gui::paths {

/**
 * @brief Gets the org config dir (parent of app config dir).
 */
inline QDir configDir(const bool persist = false)
{
  const QDir configDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));

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

/**
 * @brief Uses the Core tool to get the profile dir.
 */
inline QDir coreProfileDir()
{
  CoreTool coreTool;
  return QDir(coreTool.getProfileDir());
}

inline QString defaultTlsCertPath()
{
  const auto root = coreProfileDir();
  const auto sslDirPath = QDir(root.filePath(kSslDir));
  return sslDirPath.filePath(kCertificateFilename);
}

} // namespace deskflow::gui::paths
