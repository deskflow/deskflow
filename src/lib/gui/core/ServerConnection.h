/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>
#include <QStringList>

#include "gui/config/IAppConfig.h"
#include "gui/config/IServerConfig.h"
#include "gui/config/ServerConfigDialogState.h"
#include "gui/messages.h"

namespace deskflow::gui {

class ServerConnection : public QObject
{
  Q_OBJECT
  using IServerConfig = deskflow::gui::IServerConfig;

public:
  struct Deps
  {
    virtual ~Deps() = default;
    virtual messages::NewClientPromptResult
    showNewClientPrompt(QWidget *parent, const QString &clientName, bool serverRequiresPeerAuth = false) const;
  };

  explicit ServerConnection(
      QWidget *parent, IAppConfig &appConfig, IServerConfig &serverConfig,
      const config::ServerConfigDialogState &serverConfigDialogState,
      std::shared_ptr<Deps> deps = std::make_shared<Deps>()
  );
  void handleLogLine(const QString &logLine);

signals:
  void messageShowing();
  void configureClient(const QString &clientName);

private:
  void handleNewClient(const QString &clientName);

  QWidget *m_pParent;
  IAppConfig &m_appConfig;
  IServerConfig &m_serverConfig;
  const config::ServerConfigDialogState &m_serverConfigDialogState;
  std::shared_ptr<Deps> m_pDeps;
  QStringList m_receivedClients;
  bool m_messageShowing = false;
};

} // namespace deskflow::gui
