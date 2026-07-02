/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <optional>

namespace deskflow::common {

//! Fleet fields from a localhost coordination status reply (mesh v2).
struct LocalStatusFleet
{
  QString server;
  QString cursorHost;
  QString cursorScreen;
  QString serverIp;
  QStringList peerHosts;
};

//! One-shot poll of `127.0.0.1:port` with `{"t":"status"}`.
std::optional<LocalStatusFleet> pollLocalFleetStatus(quint16 port, int timeoutMs = 1200);

//! Connectable server candidate hosts from a status reply, excluding @p selfName.
QStringList serverHostsFromStatus(const QJsonObject &statusObject, const QString &selfName);

} // namespace deskflow::common
