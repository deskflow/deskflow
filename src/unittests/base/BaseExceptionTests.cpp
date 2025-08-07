/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "BaseExceptionTests.h"

#include "base/BaseException.h"

void BaseExceptionTests::empty()
{
  BaseException BaseException;
  const char *result = BaseException.what();

  QCOMPARE(result, "");
}

void BaseExceptionTests::nonEmpty()
{
  BaseException BaseException("test");
  const char *result = BaseException.what();

  QCOMPARE(result, "test");
}

QTEST_MAIN(BaseExceptionTests)
