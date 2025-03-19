/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerConnection.h"

#include "ServerMessage.h"

#include "common/Settings.h"

#include "Messages.h"

#include <QMessageBox>
#include <QPushButton>

namespace deskflow::gui {

//
// ServerConnection::Deps
//

messages::NewClientPromptResult ServerConnection::Deps::showNewClientPrompt(
    QWidget *parent, const QString &clientName, bool serverRequiresPeerAuth
) const
{
  return messages::showNewClientPrompt(parent, clientName, serverRequiresPeerAuth);
}

//
// ServerConnection
//

ServerConnection::ServerConnection(QWidget *parent, IServerConfig &serverConfig, std::shared_ptr<Deps> deps)
    : m_pParent(parent),
      m_serverConfig(serverConfig),
      m_pDeps(deps)
{
}

void ServerConnection::handleLogLine(const QString &logLine)
{
  ServerMessage message(logLine);

  if (!message.isNewClientMessage()) {
    return;
  }

  if (m_messageShowing) {
    qDebug("new client message already shown, skipping for now");
    return;
  }

  if (Settings::value(Settings::Server::ConfigVisible).toBool()) {
    qDebug("server config dialog visible, skipping new client prompt");
    return;
  }

  if (Settings::value(Settings::Server::ExternalConfig).toBool()) {
    qDebug("external config enabled, skipping new client prompt");
    return;
  }

  const auto client = message.getClientName();

  if (m_receivedClients.contains(client)) {
    qDebug("already got request, skipping new client prompt for: %s", qPrintable(client));
    return;
  }

  handleNewClient(message.getClientName());
}

void ServerConnection::handleNewClient(const QString &clientName)
{
  using enum messages::NewClientPromptResult;

  m_receivedClients.append(clientName);

  if (m_serverConfig.isFull()) {
    qDebug("server config full, skipping new client prompt for: %s", qPrintable(clientName));
    return;
  }

  if (m_serverConfig.screenExists(clientName)) {
    qDebug("client already added, skipping new client prompt for: %s", qPrintable(clientName));
    return;
  }

  Q_EMIT messageShowing();

  m_messageShowing = true;
  const bool tlsEnabled = Settings::value(Settings::Security::TlsEnabled).toBool();
  const bool requireCerts = Settings::value(Settings::Security::CheckPeers).toBool();
  const auto result = m_pDeps->showNewClientPrompt(m_pParent, clientName, tlsEnabled && requireCerts);
  m_messageShowing = false;

  if (result == Add) {
    qDebug("accepted dialog, adding client: %s", qPrintable(clientName));
    Q_EMIT configureClient(clientName);
  } else if (result == Ignore) {
    qDebug("declined dialog, ignoring client: %s", qPrintable(clientName));
  } else {
    qFatal("unexpected add client result");
  }
}

} // namespace deskflow::gui
