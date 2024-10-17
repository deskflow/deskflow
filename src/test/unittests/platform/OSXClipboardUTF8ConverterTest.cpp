/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2011 Nick Bolton
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
