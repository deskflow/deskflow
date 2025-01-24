/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerStateLabel.h"

#include "gui/core/ServerMessage.h"

using namespace deskflow::gui;

namespace deskflow::gui::widgets {

ServerStateLabel::ServerStateLabel(QWidget *parent) : QLabel(parent)
{
}

void ServerStateLabel::updateServerState(const QString &line)
{
  ServerMessage message(line);

  if (message.isExitMessage()) {
    m_clients.clear();
  } else if (message.isConnectedMessage()) {
    m_clients.append(message.getClientName());
  } else if (message.isDisconnectedMessage()) {
    m_clients.removeAll(message.getClientName());
  }

  if (m_clients.isEmpty()) {
    setText(tr("No clients connected"));
  } else {
    // unfortunately, we can't rely on the clients list because we don't always
    // catch the connect/disconnect messages. so clients tend to get stuck in
    // the list even though they're offline.
    // in order to properly show a list of clients, we would need the core to
    // print a list of connected clients on every connect/disconnect event,
    // which could be a bit noisy in the logs (perhaps an ipc message would be
    // needed).
    setText(tr("Client(s) are connected"));
  }
}

} // namespace deskflow::gui::widgets
