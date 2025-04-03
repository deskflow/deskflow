/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "test_String.h"

#include "../../lib/base/String.h"

#include <string.h>

void String_Test::formatWithArgs()
{
  const char *format = "%%%{1}=%{2}";
  const char *arg1 = "answer";
  const char *arg2 = "42";

  std::string result = deskflow::string::format(format, arg1, arg2);

  QCOMPARE("%answer=42", result);
}

void String_Test::formatedString()
{
  const char *format = "%s=%d";
  const char *arg1 = "answer";
  int arg2 = 42;

  std::string result = deskflow::string::sprintf(format, arg1, arg2);

  QCOMPARE("answer=42", result);
}

void String_Test::toHex()
{
  QCOMPARE("666f6f626172", deskflow::string::toHex("foobar", 2));
  std::vector<std::uint8_t> subject{'f', 'o', 'o', 'b', 'a', 'r'};
  QCOMPARE("666f6f626172", deskflow::string::toHex(subject, 2));
}

void String_Test::fromHex()
{
  QCOMPARE(-1, deskflow::string::fromHexChar('z'));
  QCOMPARE(0, deskflow::string::fromHexChar('0'));
  QCOMPARE(1, deskflow::string::fromHexChar('1'));
  QCOMPARE(2, deskflow::string::fromHexChar('2'));
  QCOMPARE(3, deskflow::string::fromHexChar('3'));
  QCOMPARE(4, deskflow::string::fromHexChar('4'));
  QCOMPARE(5, deskflow::string::fromHexChar('5'));
  QCOMPARE(6, deskflow::string::fromHexChar('6'));
  QCOMPARE(7, deskflow::string::fromHexChar('7'));
  QCOMPARE(8, deskflow::string::fromHexChar('8'));
  QCOMPARE(9, deskflow::string::fromHexChar('9'));
  QCOMPARE(10, deskflow::string::fromHexChar('a'));
  QCOMPARE(10, deskflow::string::fromHexChar('A'));
  QCOMPARE(11, deskflow::string::fromHexChar('b'));
  QCOMPARE(11, deskflow::string::fromHexChar('B'));
  QCOMPARE(12, deskflow::string::fromHexChar('c'));
  QCOMPARE(12, deskflow::string::fromHexChar('C'));
  QCOMPARE(13, deskflow::string::fromHexChar('d'));
  QCOMPARE(13, deskflow::string::fromHexChar('D'));
  QCOMPARE(14, deskflow::string::fromHexChar('e'));
  QCOMPARE(14, deskflow::string::fromHexChar('E'));
  QCOMPARE(15, deskflow::string::fromHexChar('f'));
  QCOMPARE(15, deskflow::string::fromHexChar('F'));
  QCOMPARE(255, deskflow::string::fromHex("FF")[0]);
  QCOMPARE(255, deskflow::string::fromHex(":FF:EE")[0]);
  QCOMPARE(238, deskflow::string::fromHex(":FF:EE")[1]);
}

void String_Test::toLower()
{
  std::string subject = "12foo3BaR";
  deskflow::string::uppercase(subject);
  QCOMPARE("12FOO3BAR", subject);
}

void String_Test::stringToInt()
{
  std::string number = "123";
  size_t value = deskflow::string::stringToSizeType(number);
  QCOMPARE(123, value);
}

void String_Test::intToString()
{
  size_t value = 123;
  std::string number = deskflow::string::sizeTypeToString(value);
  QCOMPARE("123", number);
}

QTEST_MAIN(String_Test)
