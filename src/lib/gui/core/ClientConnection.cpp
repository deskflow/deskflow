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

#include "ClientConnection.h"

#include "messages.h"

#include <QHostAddress>
#include <QMessageBox>

namespace deskflow::gui {

//
// ClientConnection::Deps
//

void ClientConnection::Deps::showError(QWidget *parent, messages::ClientError error, const QString &address) const
{
  messages::showClientConnectError(parent, error, address);
}

//
// ClientConnection
//

void ClientConnection::handleLogLine(const QString &logLine)
{

  if (logLine.contains("failed to connect to server")) {

    if (!m_showMessage) {
      qDebug("message already shown, skipping");
      return;
    }

    m_showMessage = false;

    // ignore the message if it's about the server refusing by name as
    // this will trigger the server to show an 'add client' dialog.
    if (logLine.contains("server refused client with our name")) {
      qDebug("ignoring client name refused message");
      return;
    }

    showMessage(logLine);
  } else if (logLine.contains("connected to server")) {
    m_showMessage = false;
  }
}

void ClientConnection::showMessage(const QString &logLine)
{
  using enum messages::ClientError;

  Q_EMIT messageShowing();

  const auto address = m_appConfig.serverHostname();

  if (logLine.contains("server already has a connected client with our name")) {
    m_deps->showError(m_pParent, AlreadyConnected, address);
  } else if (QHostAddress a(address); a.isNull()) {
    qDebug("ip not detected, showing hostname error");
    m_deps->showError(m_pParent, HostnameError, address);
  } else {
    qDebug("ip detected, showing generic error");
    m_deps->showError(m_pParent, GenericError, address);
  }
}

} // namespace deskflow::gui
