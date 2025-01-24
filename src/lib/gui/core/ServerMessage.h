/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

namespace deskflow::gui {

class ServerMessage
{
  QString m_message;
  QString m_clientName;

public:
  explicit ServerMessage(const QString &message);

  bool isNewClientMessage() const;
  bool isExitMessage() const;
  bool isConnectedMessage() const;
  bool isDisconnectedMessage() const;

  const QString &getClientName() const;

private:
  QString parseClientName(const QString &line) const;
};

} // namespace deskflow::gui
