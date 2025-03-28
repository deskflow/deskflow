/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class FingerprintTests : public QObject
{
  Q_OBJECT
private slots:
  void test_isValid();
  void test_toDbLine();
  void test_fromDbLine();
  void test_typeToString();
  void test_typeFromString();
};
