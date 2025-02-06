/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

class QLocalServer;

namespace deskflow::ipc {

class IpcServer2 : public QObject
{
  Q_OBJECT

public:
  IpcServer2(QObject *parent);
  ~IpcServer2();

private:
  QLocalServer *m_server;
};

} // namespace deskflow::ipc
