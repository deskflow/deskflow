/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2021 Symless Ltd.
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
    virtual messages::NewClientPromptResult showNewClientPrompt(QWidget *parent, const QString &clientName) const;
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
