/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
