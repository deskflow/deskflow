// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "RuntimeProfile.h"
#include <QJsonObject>
#include <QString>

namespace deskflow::lab {
// Read-only launch contract shared by the actual lab core and its tests. This
// does not create credentials, Settings, IPC, a socket or a platform screen.
struct IsolatedSession
{
  QString root, settings, state, identity, trustedServers, trustedClients, ipcName, lockFile;
  QString bindAddress, peerAddress, mode, error;
  int port = 34800;
  RuntimeProfile runtime;
  bool valid() const
  {
    return error.isEmpty() && !root.isEmpty();
  }
  static IsolatedSession inspect(const QString &root, const QString &allowedParent);
  static IsolatedSession inspectLayout(const QString &root, const QString &allowedParent);
  static bool activate(const QString &root, const QString &allowedParent);
  static const IsolatedSession *active();
  static void sealStartup(); // Settings must never switch namespaces after initialization.
  QJsonObject plan() const;
};
} // namespace deskflow::lab
