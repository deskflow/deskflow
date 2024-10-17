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

#include "ServerMessage.h"

namespace deskflow::gui {

ServerMessage::ServerMessage(const QString &message) : m_message(message), m_clientName(parseClientName(message))
{
}

bool ServerMessage::isNewClientMessage() const
{
  return m_message.contains("unrecognised client name");
}

bool ServerMessage::isExitMessage() const
{
  return m_message.contains("process exited");
}

bool ServerMessage::isConnectedMessage() const
{
  return m_message.contains("has connected");
}

bool ServerMessage::isDisconnectedMessage() const
{
  return m_message.contains("has disconnected");
}

const QString &ServerMessage::getClientName() const
{
  return m_clientName;
}

QString ServerMessage::parseClientName(const QString &line) const
{
  QString clientName("Unknown");
  auto nameStart = line.indexOf('"') + 1;
  auto nameEnd = line.indexOf('"', nameStart);

  if (nameEnd > nameStart) {
    clientName = line.mid(nameStart, nameEnd - nameStart);
  }

  return clientName;
}

} // namespace deskflow::gui
