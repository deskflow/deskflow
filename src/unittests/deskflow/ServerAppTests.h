/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"
#include "base/Log.h"

#include <QTest>

class ServerAppTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void handleResume_startFails_retriesUntilStarted();
  void handleSuspend_whileRetrying_stopsRetrying();

private:
  Arch m_arch;
  Log m_log;
};
