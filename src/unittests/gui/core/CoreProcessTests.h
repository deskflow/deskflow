/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class CoreProcessTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void parseComputerShape_data();
  void parseComputerShape();
};
