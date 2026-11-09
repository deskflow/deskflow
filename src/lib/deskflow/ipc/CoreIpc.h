/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"
#include "common/Enums.h"
#include <QByteArray>

#include <QString>

void ipcSendToClient(const QString &command, const QString &args = "");
void ipcSendConnectionState(deskflow::core::ConnectionState state);
struct WorkspaceApplyEvent final : EventData
{
  QByteArray request;
  explicit WorkspaceApplyEvent(QByteArray value) : request(std::move(value))
  {
  }
};
