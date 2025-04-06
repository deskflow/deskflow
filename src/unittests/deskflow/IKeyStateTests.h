/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class IKeyStateTests : public QObject
{
  Q_OBJECT
private slots:
  void allocDestination();

private:
  Arch m_arch;
  Log m_log;
};
