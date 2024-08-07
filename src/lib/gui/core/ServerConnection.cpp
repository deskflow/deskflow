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
#include <QPushButton>

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
  const QPushButton *ignore =
      message.addButton("Ignore", QMessageBox::RejectRole);
  const QPushButton *add =
      message.addButton("Add client", QMessageBox::AcceptRole);
  message.setText(
      QObject::tr("A new client called '%1' wants to connect").arg(clientName));
  message.exec();

  if (message.clickedButton() == add) {
    qDebug("accepted dialog, adding client: %s", qPrintable(clientName));
    emit configureClient(clientName);
  } else if (message.clickedButton() == ignore) {
    qDebug("declined dialog, ignoring client: %s", qPrintable(clientName));
    m_ignoredClients.append(clientName);
  } else {
    qFatal("no expected dialog button was clicked");
  }
}

} // namespace synergy::gui
