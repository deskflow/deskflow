/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "test_XBase.h"

#include "base/XBase.h"

void XBase_Test::empty()
{
  XBase xbase;
  const char *result = xbase.what();
  QCOMPARE(result, "");
}

void XBase_Test::nonEmpty()
{
  XBase xbase("test");
  const char *result = xbase.what();
  QCOMPARE(result, "test");
}

QTEST_MAIN(XBase_Test)
