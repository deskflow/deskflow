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

#include "ClientConnection.h"

#include <QHostAddress>
#include <QMessageBox>
#include <qglobal.h>

namespace synergy::gui {

ClientConnection::ClientConnection(QWidget &parent, AppConfig &appConfig)
    : m_parent(parent),
      m_appConfig(appConfig) {}

void ClientConnection::update(const QString &line) {
  if (m_checkConnection && checkMainWindow()) {
    if (line.contains("failed to connect to server")) {
      m_checkConnection = false;
      if (!line.contains("server refused client with our name") &&
          !line.contains("Trying next address")) {
        showMessage(getMessage(line));
      }
    } else if (line.contains("connected to server")) {
      m_checkConnection = false;
    }
  }
}

bool ClientConnection::checkMainWindow() {
  bool result = m_parent.isActiveWindow();

  if (m_parent.isMinimized() || m_parent.isHidden()) {
    m_parent.showNormal();
    m_parent.activateWindow();
    result = true;
  }

  return result;
}

QString ClientConnection::getMessage(const QString &line) const {
  QHostAddress address(m_appConfig.serverHostname());
  auto message = QString("<p>The connection to server '%1' didn't work.</p>")
                     .arg(m_appConfig.serverHostname());

  if (line.contains("server already has a connected client with our name")) {
    return message +
           "<p>Two of your client computers have the same name or there are "
           "two instances of the client process running.</p>"
           "<p>Please ensure that you're using a unique name and that only a "
           "single client process is running.</p>";
  } else if (address.isNull()) {
    return message + "<p>Please try to connect to the server using the "
                     "server IP address instead of the hostname. </p>"
                     "<p>If that doesn't work, please check your TLS and "
                     "firewall settings.</p>";
  } else {
    return message + "<p>Please check your TLS and firewall settings.</p>";
  }
}

void ClientConnection::showMessage(const QString &message) const {
  QMessageBox dialog(&m_parent);
  dialog.addButton(QObject::tr("Close"), QMessageBox::RejectRole);
  dialog.setText(message);
  dialog.exec();
}

void ClientConnection::setCheckConnection(bool checkConnection) {
  m_checkConnection = checkConnection;
}

} // namespace synergy::gui
