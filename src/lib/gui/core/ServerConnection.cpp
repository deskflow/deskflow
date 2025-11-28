/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerConnection.h"

#include "ServerMessage.h"
#include "common/Settings.h"

#include <QMessageBox>
#include <QPushButton>

namespace deskflow::gui {

ServerConnection::ServerConnection(QWidget *parent, IServerConfig &serverConfig)
    : m_pParent(parent),
      m_serverConfig(serverConfig)
{
}

void ServerConnection::handleLogLine(const QString &logLine)
{
  ServerMessage message(logLine);
  const auto &clientName = message.getClientName();

  if (m_ignoredClients.contains(clientName)) {
    qDebug("ignoring %s:", qPrintable(clientName));
    return;
  }

  if (message.isDisconnectedMessage()) {
    m_connectedClients.remove(clientName);
    Q_EMIT clientsChanged(connectedClients());
    return;
  }

  if (message.isConnectedMessage()) {
    m_connectedClients.insert(clientName);
    Q_EMIT clientsChanged(connectedClients());
    return;
  }

  if (!message.isNewClientMessage()) {
    return;
  }

  if (m_messageShowing) {
    qDebug("new client message already shown, skipping for now");
    return;
  }

  if (m_serverConfigDialogVisible) {
    qDebug("server config dialog visible, skipping new client prompt");
    return;
  }

  if (Settings::value(Settings::Server::ExternalConfig).toBool()) {
    qDebug("external config enabled, skipping new client prompt");
    return;
  }

  if (m_connectedClients.contains(clientName)) {
    qDebug("already got request, skipping new client prompt for: %s", qPrintable(clientName));
    return;
  }

  handleNewClient(clientName);
}

void ServerConnection::handleNewClient(const QString &clientName)
{
  if (m_serverConfig.isFull()) {
    qDebug("server config full, skipping new client prompt for: %s", qPrintable(clientName));
    return;
  }

  if (m_serverConfig.screenExists(clientName)) {
    qDebug("client already added, skipping new client prompt for: %s", qPrintable(clientName));
    return;
  }

  m_messageShowing = true;
  const bool tlsEnabled = Settings::value(Settings::Security::TlsEnabled).toBool();
  const bool requireCerts = Settings::value(Settings::Security::CheckPeers).toBool();
  Q_EMIT requestNewClientPrompt(clientName, tlsEnabled && requireCerts);
}

void ServerConnection::handleNewClientResult(const QString &clientName, bool acceptClient)
{
  m_messageShowing = false;
  if (!acceptClient) {
    qDebug("declined dialog, ignoring client: %s", qPrintable(clientName));
    m_ignoredClients.insert(clientName);
    return;
  }

  qDebug("accepted dialog, adding client: %s", qPrintable(clientName));
  Q_EMIT configureClient(clientName);
  m_connectedClients.insert(clientName);
  Q_EMIT clientsChanged(connectedClients());
}

QStringList ServerConnection::connectedClients() const
{
  return QStringList(m_connectedClients.begin(), m_connectedClients.end());
}

} // namespace deskflow::gui
