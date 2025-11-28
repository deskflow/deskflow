/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

#include "gui/Messages.h"
#include "gui/config/IServerConfig.h"

namespace deskflow::gui {

class ServerConnection : public QObject
{
  Q_OBJECT
  using IServerConfig = deskflow::gui::IServerConfig;

public:
  explicit ServerConnection(QWidget *parent, IServerConfig &serverConfig);
  void handleLogLine(const QString &logLine);
  void serverConfigDialogVisible(bool visible)
  {
    m_serverConfigDialogVisible = visible;
  }

  QStringList connectedClients() const;
  void handleNewClientResult(const QString &clientName, bool acceptClient);

Q_SIGNALS:
  void requestNewClientPrompt(const QString &clientName, bool peerAuthRequired);
  void configureClient(const QString &clientName);
  void clientsChanged(const QStringList &clients);

private:
  void handleNewClient(const QString &clientName);

  QWidget *m_pParent;
  IServerConfig &m_serverConfig;
  QSet<QString> m_connectedClients;
  QSet<QString> m_ignoredClients;
  bool m_messageShowing = false;
  bool m_serverConfigDialogVisible = false;
};

} // namespace deskflow::gui
