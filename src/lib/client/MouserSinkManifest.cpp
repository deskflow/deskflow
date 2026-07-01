/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/MouserSinkManifest.h"

#include "base/Log.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace deskflow::client {

namespace {

QString manifestPath()
{
  const auto configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  if (configDir.isEmpty()) {
    return {};
  }
  return QDir(configDir).filePath(QStringLiteral("mouser-sink.json"));
}

} // namespace

void writeMouserSinkManifest(int port, const QString &token)
{
  const auto path = manifestPath();
  if (path.isEmpty() || token.isEmpty()) {
    return;
  }

  QJsonObject root;
  root[QStringLiteral("version")] = 1;
  root[QStringLiteral("port")] = port;
  root[QStringLiteral("token")] = token;
  root[QStringLiteral("protocol")] = QStringLiteral("dfhr+v1");

  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    LOG_WARN("mouser sink manifest: failed to write %s", path.toUtf8().constData());
    return;
  }
  file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
  file.close();
  LOG_INFO("mouser sink manifest: wrote %s", path.toUtf8().constData());
}

void clearMouserSinkManifest()
{
  const auto path = manifestPath();
  if (path.isEmpty()) {
    return;
  }
  if (QFile::remove(path)) {
    LOG_INFO("mouser sink manifest: removed %s", path.toUtf8().constData());
  }
}

} // namespace deskflow::client
