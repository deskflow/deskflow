/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoordinationStatus.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <QTimer>

#include <memory>

namespace deskflow::gui {

namespace {

//! "macbookpro.tail.ts.net" / "macbookpro.lan" -> "macbookpro".
QString shortName(const QString &address)
{
  const auto dot = address.indexOf(QLatin1Char('.'));
  return dot < 0 ? address : address.left(dot);
}

} // namespace

QString formatFleetGraph(const QJsonObject &fleet)
{
  if (fleet.isEmpty()) {
    return {};
  }

  QStringList screenNames;
  const QJsonArray screens = fleet[QStringLiteral("screens")].toArray();
  screenNames.reserve(screens.size());
  for (const auto &screen : screens) {
    screenNames << screen.toString();
  }
  if (screenNames.isEmpty()) {
    return {};
  }

  QStringList edges;
  const QJsonArray links = fleet[QStringLiteral("links")].toArray();
  edges.reserve(links.size());
  for (const auto &linkValue : links) {
    const QJsonObject link = linkValue.toObject();
    const QString from = link[QStringLiteral("from")].toString();
    const QString to = link[QStringLiteral("to")].toString();
    if (from.isEmpty() || to.isEmpty()) {
      continue;
    }
    const QString dir = link[QStringLiteral("dir")].toString();
    if (dir.isEmpty()) {
      edges << QStringLiteral("%1→%2").arg(from, to);
    } else {
      edges << QStringLiteral("%1→%2 (%3)").arg(from, to, dir);
    }
  }

  QString graph = screenNames.join(QStringLiteral(" · "));
  if (!edges.isEmpty()) {
    graph += QStringLiteral(" — ") + edges.join(QStringLiteral(", "));
  }
  return graph;
}

QString formatStatusAnnotation(const QJsonObject &status)
{
  const int meshVersion = status[QStringLiteral("mesh_version")].toInt();
  const QJsonArray mismatches = status[QStringLiteral("version_mismatch")].toArray();
  if (!mismatches.isEmpty()) {
    QStringList names;
    names.reserve(mismatches.size());
    for (const auto &value : mismatches) {
      if (value.isString()) {
        names << value.toString();
      }
    }
    if (!names.isEmpty()) {
      return QStringLiteral("mesh version mismatch: %1").arg(names.join(QStringLiteral(", ")));
    }
  }
  if (meshVersion >= 2 && status[QStringLiteral("role")].toString() == QStringLiteral("client") &&
      formatFleetGraph(status[QStringLiteral("fleet")].toObject()).isEmpty()) {
    return QStringLiteral("awaiting fleet topology");
  }
  return {};
}

CoordinationStatus::CoordinationStatus(QObject *parent) : QObject(parent), m_timer(new QTimer(this))
{
  connect(m_timer, &QTimer::timeout, this, &CoordinationStatus::poll);
}

void CoordinationStatus::start(quint16 port, int intervalMs)
{
  m_port = port;
  m_timer->start(intervalMs);
  poll(); // immediate first reading
}

void CoordinationStatus::stop()
{
  m_timer->stop();
}

void CoordinationStatus::poll()
{
  auto *socket = new QTcpSocket(this);
  auto done = std::make_shared<bool>(false);

  // One-shot query: connect, send status, read one JSON line, done. Guarded
  // so readyRead and the timeout can't both resolve it; the socket cleans
  // itself up on any terminal outcome.
  const auto finish = [this, socket, done](
                          bool ok, const QString &role = {}, const QString &serverName = {},
                          const QString &fleetGraph = {}
                      ) {
    if (*done)
      return;
    *done = true;
    socket->disconnect();
    socket->abort();
    socket->deleteLater();
    if (ok)
      Q_EMIT online(role, serverName, fleetGraph);
    else
      Q_EMIT offline();
  };

  connect(socket, &QTcpSocket::connected, this, [socket] {
    socket->write("{\"t\":\"status\"}\n");
  });

  connect(socket, &QTcpSocket::readyRead, this, [socket, finish] {
    const auto doc = QJsonDocument::fromJson(socket->readLine());
    if (!doc.isObject()) {
      finish(false);
      return;
    }
    const QJsonObject obj = doc.object();
    const QString role = obj[QStringLiteral("role")].toString();
    const QString server = shortName(obj[QStringLiteral("server_ip")].toString());
    QString fleetGraph = formatFleetGraph(obj[QStringLiteral("fleet")].toObject());
    const QString annotation = formatStatusAnnotation(obj);
    if (!annotation.isEmpty()) {
      fleetGraph = fleetGraph.isEmpty() ? annotation : QStringLiteral("%1 — %2").arg(fleetGraph, annotation);
    }
    finish(!role.isEmpty(), role, server, fleetGraph);
  });

  connect(socket, &QTcpSocket::errorOccurred, this, [finish] { finish(false); });

  // Bound the wait so a missing coordinator resolves to "offline" promptly.
  QTimer::singleShot(1200, socket, [socket, finish] {
    if (socket->state() != QAbstractSocket::ConnectedState || socket->bytesAvailable() == 0)
      finish(false);
  });

  socket->connectToHost(QHostAddress::LocalHost, m_port);
}

} // namespace deskflow::gui
