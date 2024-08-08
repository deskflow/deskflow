/*
 * synergy -- mouse and keyboard sharing utility
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

#include "ServerConnection.h"

#include "ServerMessage.h"
#include "messages.h"

#include <QMessageBox>
#include <QPushButton>

namespace synergy::gui {

//
// ServerConnection::Deps
//

messages::NewClientPromptResult ServerConnection::Deps::showNewClientPrompt(
    QWidget *parent, const QString &clientName) const {
  return messages::showNewClientPrompt(parent, clientName);
}

//
// ServerConnection
//

ServerConnection::ServerConnection(
    QWidget *parent, IAppConfig &appConfig, IServerConfig &serverConfig,
    std::shared_ptr<Deps> deps)
    : m_pParent(parent),
      m_appConfig(appConfig),
      m_serverConfig(serverConfig),
      m_pDeps(deps) {}

void ServerConnection::handleLogLine(const QString &logLine) {
  ServerMessage message(logLine);

  if (!m_appConfig.useExternalConfig() && message.isNewClientMessage() &&
      !m_ignoredClients.contains(message.getClientName())) {
    handleNewClient(message.getClientName());
  }
}

void ServerConnection::handleNewClient(const QString &clientName) {
  using enum messages::NewClientPromptResult;

  if (m_serverConfig.isFull()) {
    qDebug(
        "server config full, skipping add client prompt for: %s",
        qPrintable(clientName));
    return;
  }

  if (m_serverConfig.screenExists(clientName)) {
    qDebug(
        "client already added, skipping add client prompt for: %s",
        qPrintable(clientName));
    return;
  }

  emit messageShowing();

  const auto result = m_pDeps->showNewClientPrompt(m_pParent, clientName);
  if (result == Add) {
    qDebug("accepted dialog, adding client: %s", qPrintable(clientName));
    emit configureClient(clientName);
  } else if (result == Ignore) {
    qDebug("declined dialog, ignoring client: %s", qPrintable(clientName));
    m_ignoredClients.append(clientName);
  } else {
    qFatal("unexpected add client result");
  }
}

} // namespace synergy::gui
