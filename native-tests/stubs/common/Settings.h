/*
 * Deskflow -- isolated widget test support
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>
#include <QVariant>

// This test-only adapter has no QSettings, file access, migration or singleton.
class Settings
{
public:
  struct Client
  {
    inline static const auto DynamicConnectionRetry = QStringLiteral("client/dynamicConnectionInterval");
    inline static const auto RemoteHost = QStringLiteral("client/remoteHost");
  };

  static QVariant value(const QString &key)
  {
    if (key == Client::DynamicConnectionRetry)
      return true;
    if (key == Client::RemoteHost)
      return QStringLiteral("example-server.invalid");
    qFatal("unexpected settings access in isolated status bar test");
    return {};
  }
};
