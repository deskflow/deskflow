/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QObject>
#include <QTest>

class SecureUtils_Test : public QObject
{
  Q_OBJECT
private slots:
  void checkHex();
  void checkArt();
};
