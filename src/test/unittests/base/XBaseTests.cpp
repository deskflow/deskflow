/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/XBase.h"

#include <gtest/gtest.h>

TEST(XBaseTests, what_emptyWhat_returnsWhatFromGetWhat)
{
  XBase xbase;

  const char *result = xbase.what();

  EXPECT_STREQ("", result);
}

TEST(XBaseTests, what_nonEmptyWhat_returnsWhatFromGetWhat)
{
  XBase xbase("test");

  const char *result = xbase.what();

  EXPECT_STREQ("test", result);
}
