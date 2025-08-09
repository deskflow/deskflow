/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: 2023 Arjen Hiemstra <ahiemstra@heimr.nl>
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#include "PortalRequest.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusPendingReply>

namespace deskflow {
void PortalRequest::onStarted(QDBusPendingCallWatcher *watcher)
{
  QDBusPendingReply<QDBusObjectPath> reply = *watcher;
  if (!reply.isError()) {
    QDBusConnection::sessionBus().connect(
        QString{}, reply.value().path(), "org.freedesktop.portal.Request", "Response", this,
        SLOT(onFinished(uint, QVariantMap))
    );
  } else {
    qInfo() << "ERROR:" << reply.error().message();
    m_callback(-1, {{QStringLiteral("errorMessage"), reply.error().message()}});
  }
  watcher->deleteLater();
}

void PortalRequest::onFinished(uint code, const QVariantMap &result)
{
  if (m_context) {
    m_callback(code, result);
  }
  deleteLater();
}
} // namespace deskflow
