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
#include "gui/config/AppConfig.h"

#include <QMessageBox>

namespace synergy::gui {

ServerConnection::ServerConnection(
    QWidget &parent, AppConfig &appConfig, IServerConfig &serverConfig)
    : m_parent(parent),
      m_appConfig(appConfig),
      m_serverConfig(serverConfig) {}

void ServerConnection::update(const QString &line) {
  ServerMessage message(line);

  if (!m_appConfig.useExternalConfig() && message.isNewClientMessage() &&
      !m_ignoredClients.contains(message.getClientName())) {
    addClient(message.getClientName());
  }
}

// TOOD: merge duplicated code between client and server connection
bool ServerConnection::checkMainWindow() {
  bool result = m_parent.isActiveWindow();

  if (m_parent.isMinimized() || m_parent.isHidden()) {
    m_parent.showNormal();
    m_parent.activateWindow();
    result = true;
  }

  return result;
}

void ServerConnection::addClient(const QString &clientName) {
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

  if (!checkMainWindow()) {
    qDebug(
        "main window not active, skipping add client prompt for: %s",
        qPrintable(clientName));
    return;
  }

  QMessageBox message(&m_parent);
  message.addButton(QObject::tr("Ignore"), QMessageBox::RejectRole);
  message.addButton(QObject::tr("Add client"), QMessageBox::AcceptRole);
  message.setText(
      QObject::tr("Client with name '%1' wants to connect").arg(clientName));

  if (message.exec() == QMessageBox::Accepted) {
    emit configureClient(clientName);
  } else {
    m_ignoredClients.append(clientName);
  }
}

} // namespace synergy::gui
