/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

#include "base/String.h"

#include <gtest/gtest.h>

using namespace deskflow;

TEST(StringTests, format_formatWithArguments_formatedString)
{
  const char *format = "%%%{1}=%{2}";
  const char *arg1 = "answer";
  const char *arg2 = "42";

  std::string result = string::format(format, arg1, arg2);

  EXPECT_EQ("%answer=42", result);
}

TEST(StringTests, findReplaceAll_inputString_replacedString)
{
  std::string subject = "foobar";
  std::string find = "bar";
  std::string replace = "baz";

  string::findReplaceAll(subject, find, replace);

  EXPECT_EQ("foobaz", subject);
}

TEST(StringTests, sprintf_formatWithArgument_formatedString)
{
  const char *format = "%s=%d";
  const char *arg1 = "answer";
  int arg2 = 42;

  std::string result = string::sprintf(format, arg1, arg2);

  EXPECT_EQ("answer=42", result);
}

TEST(StringTests, toHex_plaintext_hexString)
{
  EXPECT_EQ("666f6f626172", string::toHex("foobar", 2));
}

TEST(StringTests, uppercase_lowercaseInput_uppercaseOutput)
{
  std::string subject = "12foo3BaR";

  string::uppercase(subject);

  EXPECT_EQ("12FOO3BAR", subject);
}

TEST(StringTests, removeChar_inputString_removeAllSpecifiedCharactors)
{
  std::string subject = "foobar";
  const char c = 'o';

  string::removeChar(subject, c);

  EXPECT_EQ("fbar", subject);
}

TEST(StringTests, intToString_inputInt_outputString)
{
  size_t value = 123;

  std::string number = string::sizeTypeToString(value);

  EXPECT_EQ("123", number);
}

TEST(StringTests, stringToUint_inputString_outputInt)
{
  std::string number = "123";

  size_t value = string::stringToSizeType(number);

  EXPECT_EQ(123, value);
}

TEST(StringTests, splitString_twoSeparator_returnThreeParts)
{
  std::string string = "stub1:stub2:stub3";

  std::vector<std::string> results = string::splitString(string, ':');

  EXPECT_EQ(3, results.size());
  EXPECT_EQ("stub1", results[0]);
  EXPECT_EQ("stub2", results[1]);
  EXPECT_EQ("stub3", results[2]);
}

TEST(StringTests, splitString_oneSeparator_returnTwoParts)
{
  std::string string = "stub1:stub2";

  std::vector<std::string> results = string::splitString(string, ':');

  EXPECT_EQ(2, results.size());
  EXPECT_EQ("stub1", results[0]);
  EXPECT_EQ("stub2", results[1]);
}

TEST(StringTests, splitString_noSeparator_returnOriginalString)
{
  std::string string = "stub1";

  std::vector<std::string> results = string::splitString(string, ':');

  EXPECT_EQ(1, results.size());
  EXPECT_EQ("stub1", results[0]);
}

TEST(StringTests, splitString_emptyString_returnEmptyVector)
{
  std::string string;

  std::vector<std::string> results = string::splitString(string, ':');

  EXPECT_EQ(0, results.size());
}

TEST(StringTests, splitString_tailSeparator_returnTwoParts)
{
  std::string string = "stub1:stub2:";

  std::vector<std::string> results = string::splitString(string, ':');

  EXPECT_EQ(2, results.size());
  EXPECT_EQ("stub1", results[0]);
  EXPECT_EQ("stub2", results[1]);
}

TEST(StringTests, splitString_headSeparator_returnTwoParts)
{
  std::string string = ":stub1:stub2";

  std::vector<std::string> results = string::splitString(string, ':');

  EXPECT_EQ(2, results.size());
  EXPECT_EQ("stub1", results[0]);
  EXPECT_EQ("stub2", results[1]);
}

TEST(StringTests, splitString_headAndTailSeparators_returnTwoParts)
{
  std::string string = ":stub1:stub2:";

  std::vector<std::string> results = string::splitString(string, ':');

  EXPECT_EQ(2, results.size());
  EXPECT_EQ("stub1", results[0]);
  EXPECT_EQ("stub2", results[1]);
}
