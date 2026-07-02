/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "common/CoordinationLocalStatus.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>

namespace deskflow::common {

namespace {

void appendUnique(QStringList &hosts, const QString &host)
{
  const auto trimmed = host.trimmed();
  if (trimmed.isEmpty() || hosts.contains(trimmed)) {
    return;
  }
  hosts.append(trimmed);
}

} // namespace

QStringList serverHostsFromStatus(const QJsonObject &statusObject, const QString &selfName)
{
  QStringList hosts;
  appendUnique(hosts, statusObject[QStringLiteral("server_ip")].toString());

  const QJsonObject fleet = statusObject[QStringLiteral("fleet")].toObject();
  const QString fleetServer = fleet[QStringLiteral("server")].toString();
  if (!fleetServer.isEmpty() && fleetServer.compare(selfName, Qt::CaseInsensitive) != 0) {
    appendUnique(hosts, fleetServer);
  }

  const QJsonArray peers = fleet[QStringLiteral("peers")].toArray();
  for (const auto &value : peers) {
    const QJsonObject peer = value.toObject();
    const QString name = peer[QStringLiteral("name")].toString();
    if (name.compare(selfName, Qt::CaseInsensitive) == 0) {
      continue;
    }
    appendUnique(hosts, peer[QStringLiteral("ip")].toString());
    appendUnique(hosts, peer[QStringLiteral("lan")].toString());
    appendUnique(hosts, name);
  }
  return hosts;
}

std::optional<LocalStatusFleet> pollLocalFleetStatus(quint16 port, int timeoutMs)
{
  QTcpSocket socket;
  socket.connectToHost(QHostAddress::LocalHost, port);
  if (!socket.waitForConnected(timeoutMs)) {
    return std::nullopt;
  }

  socket.write("{\"t\":\"status\"}\n");
  if (!socket.waitForReadyRead(timeoutMs)) {
    return std::nullopt;
  }

  const QByteArray line = socket.readLine();
  const auto doc = QJsonDocument::fromJson(line);
  if (!doc.isObject()) {
    return std::nullopt;
  }

  const QJsonObject object = doc.object();
  if (object[QStringLiteral("role")].toString().isEmpty()) {
    return std::nullopt;
  }

  const QJsonObject fleet = object[QStringLiteral("fleet")].toObject();
  LocalStatusFleet snapshot;
  snapshot.server = fleet[QStringLiteral("server")].toString();
  snapshot.cursorHost = fleet[QStringLiteral("cursor_host")].toString();
  snapshot.cursorScreen = fleet[QStringLiteral("cursor_screen")].toString();
  snapshot.serverIp = object[QStringLiteral("server_ip")].toString();
  snapshot.peerHosts = serverHostsFromStatus(object, object[QStringLiteral("name")].toString());
  return snapshot;
}

} // namespace deskflow::common
