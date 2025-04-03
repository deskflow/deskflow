/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class UnicodeTests : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase();
  void UTF32ToUTF8();
  void UTF16ToUTF8();
  void UCS2ToUTF8_kUCS2();
  void UCS2ToUTF8();

private:
  Arch m_arch;
  Log m_log;
};
