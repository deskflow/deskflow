/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
