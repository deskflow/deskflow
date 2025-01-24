/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/IArchString.h"
#include "base/Unicode.h"

#include <gtest/gtest.h>

TEST(UnicodeTests, doUTF32ToUTF8_will_convert_simple_string)
{
  bool errors;
  auto result = Unicode::UTF32ToUTF8(std::string("h\0\0\0e\0\0\0l\0\0\0l\0\0\0o\0\0\0", 20), &errors);
  EXPECT_FALSE(errors);
  EXPECT_STREQ(result.c_str(), "hello");
}

TEST(UnicodeTests, doUTF16ToUTF8_will_convert_simple_string)
{
  bool errors;
  auto result = Unicode::UTF16ToUTF8(std::string("h\0e\0l\0l\0o\0", 10), &errors);
  EXPECT_FALSE(errors);
  EXPECT_STREQ(result.c_str(), "hello");
}

TEST(UnicodeTests, doUCS2ToUTF8_will_convert_simple_string_kUCS2)
{
  bool errors;
  auto result = Unicode::textToUTF8("hello", &errors, IArchString::kUCS2);
  EXPECT_FALSE(errors);
#ifdef _WIN32
  EXPECT_EQ(result, std::string("hello", 5)); // mixed-platform expected result
#else
  EXPECT_EQ(result, std::string("h\0e\0l", 5)); // mixed-platform expected result
#endif // _WIN32
}

TEST(UnicodeTests, doUCS2ToUTF8_will_convert_simple_string_any_platform)
{
  bool errors;
  auto result = Unicode::textToUTF8("hello", &errors);
  EXPECT_FALSE(errors);
  EXPECT_EQ(result, std::string("hello", 5)); // mixed-platform expected result
}
