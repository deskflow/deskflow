/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXClipboardUTF8Converter.h"

#include <gtest/gtest.h>

TEST(OSXClipboardUTF8ConverterTests, test_Format)
{
  OSXClipboardUTF8Converter converter;
  EXPECT_EQ(IClipboard::kText, converter.getFormat());
  EXPECT_EQ(CFSTR("public.utf8-plain-text"), converter.getOSXFormat());
}

TEST(OSXClipboardUTF8ConverterTests, test_readWriteClipboard)
{
  OSXClipboardUTF8Converter converter;
  EXPECT_EQ("test data\r", converter.fromIClipboard("test data\n"));
  EXPECT_EQ("test data\n", converter.toIClipboard("test data\r"));
}
