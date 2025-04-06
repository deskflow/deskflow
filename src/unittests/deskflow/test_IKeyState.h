/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "../../lib/deskflow/IKeyState.h"

#include <QObject>
#include <QTest>

#include "base/Log.h"

class IKeyState_Test : public QObject
{
  Q_OBJECT
private slots:
  void allocdestination();

private:
  Arch m_arch;
  Log m_log;
};
