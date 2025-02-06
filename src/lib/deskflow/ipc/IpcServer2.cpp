/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IpcServer2.h"

#include "base/Log.h"

#include <QLocalServer>

namespace deskflow::ipc {

const auto kServerName = QStringLiteral("deskflow-daemon");

IpcServer2::IpcServer2(QObject *parent) : QObject(parent), m_server{new QLocalServer(this)}
{
  m_server->removeServer(kServerName);
  m_server->listen(kServerName);
  LOG_INFO("ipc server listening on %s", kServerName.toStdString().c_str());
}

IpcServer2::~IpcServer2()
{
  m_server->close();
}

} // namespace deskflow::ipc
