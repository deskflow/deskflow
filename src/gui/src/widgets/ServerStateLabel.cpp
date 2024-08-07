/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2021 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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

#include "ServerStateLabel.h"

#include "gui/core/ServerMessage.h"

using namespace synergy::gui;

namespace synergy_widgets {

ServerStateLabel::ServerStateLabel(QWidget *parent) : QLabel(parent) {}

void ServerStateLabel::updateServerState(const QString &line) {
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

} // namespace synergy_widgets
