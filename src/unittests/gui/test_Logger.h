/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

#include "../../lib/gui/Logger.h"

class Logger_Test : public QObject
{
  Q_OBJECT
private slots:
  // Test are run in order top to bottom
  void newLine();
  void noNewLine();
};
