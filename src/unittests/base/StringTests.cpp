/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "StringTests.h"

#include "../../lib/base/String.h"

#include <string.h>

void StringTests::formatWithArgs()
{
  const char *format = "%%%{1}=%{2}";
  const char *arg1 = "answer";
  const char *arg2 = "42";

  std::string result = deskflow::string::format(format, arg1, arg2);

  QCOMPARE(result, "%answer=42");
}

void StringTests::formatedString()
{
  const char *format = "%s=%d";
  const char *arg1 = "answer";
  int arg2 = 42;

  std::string result = deskflow::string::sprintf(format, arg1, arg2);

  QCOMPARE("answer=42", result);
}

void StringTests::toHex()
{
  QCOMPARE(deskflow::string::toHex("foobar", 2), "666f6f626172");

  std::vector<std::uint8_t> subject{'f', 'o', 'o', 'b', 'a', 'r'};
  QCOMPARE(deskflow::string::toHex(subject, 2), "666f6f626172");
}

void StringTests::fromHex()
{
  QCOMPARE(deskflow::string::fromHexChar('z'), -1);
  QCOMPARE(deskflow::string::fromHexChar('0'), 0);
  QCOMPARE(deskflow::string::fromHexChar('1'), 1);
  QCOMPARE(deskflow::string::fromHexChar('2'), 2);
  QCOMPARE(deskflow::string::fromHexChar('3'), 3);
  QCOMPARE(deskflow::string::fromHexChar('4'), 4);
  QCOMPARE(deskflow::string::fromHexChar('5'), 5);
  QCOMPARE(deskflow::string::fromHexChar('6'), 6);
  QCOMPARE(deskflow::string::fromHexChar('7'), 7);
  QCOMPARE(deskflow::string::fromHexChar('8'), 8);
  QCOMPARE(deskflow::string::fromHexChar('9'), 9);
  QCOMPARE(deskflow::string::fromHexChar('a'), 10);
  QCOMPARE(deskflow::string::fromHexChar('A'), 10);
  QCOMPARE(deskflow::string::fromHexChar('b'), 11);
  QCOMPARE(deskflow::string::fromHexChar('B'), 11);
  QCOMPARE(deskflow::string::fromHexChar('c'), 12);
  QCOMPARE(deskflow::string::fromHexChar('C'), 12);
  QCOMPARE(deskflow::string::fromHexChar('d'), 13);
  QCOMPARE(deskflow::string::fromHexChar('D'), 13);
  QCOMPARE(deskflow::string::fromHexChar('e'), 14);
  QCOMPARE(deskflow::string::fromHexChar('E'), 14);
  QCOMPARE(deskflow::string::fromHexChar('f'), 15);
  QCOMPARE(deskflow::string::fromHexChar('F'), 15);
  QCOMPARE(deskflow::string::fromHex("FF")[0], 255);
  QCOMPARE(deskflow::string::fromHex(":FF:EE")[0], 255);
  QCOMPARE(deskflow::string::fromHex(":FF:EE")[1], 238);
}

void StringTests::toLower()
{
  std::string subject = "12foo3BaR";
  deskflow::string::uppercase(subject);
  QCOMPARE(subject, "12FOO3BAR");
}

void StringTests::stringToInt()
{
  std::string number = "123";
  size_t value = deskflow::string::stringToSizeType(number);
  QCOMPARE(value, 123);
}

void StringTests::intToString()
{
  size_t value = 123;
  std::string number = deskflow::string::sizeTypeToString(value);
  QCOMPARE(number, "123");
}

QTEST_MAIN(StringTests)
