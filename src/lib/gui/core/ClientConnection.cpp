/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClientConnection.h"

#include "Messages.h"
#include "common/Settings.h"

#include <QHostAddress>
#include <QMessageBox>

namespace deskflow::gui {

//
// ClientConnection::Deps
//

void ClientConnection::Deps::showError(QWidget *parent, deskflow::client::ErrorType error, const QString &address) const
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
  using enum deskflow::client::ErrorType;

  if (logLine.isEmpty())
    return;

  const auto address = Settings::value(Settings::Client::RemoteHost).toString();
  auto error = NoError;

  if (logLine.contains("server already has a connected client with our name")) {
    error = AlreadyConnected;
  } else if (QHostAddress a(address); a.isNull()) {
    qDebug("ip not detected, showing hostname error");
    error = HostnameError;
  } else {
    qDebug("ip detected, showing generic error");
    error = GenericError;
  }

  if (error == NoError)
    return;

  Q_EMIT requestShowError(error, address);
  Q_EMIT messageShowing();
  m_deps->showError(m_pParent, error, address);
}

} // namespace deskflow::gui
