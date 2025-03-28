/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FingerprintTests.h"

#include "net/Fingerprint.h"

void FingerprintTests::test_isValid()
{
  Fingerprint f;

  QVERIFY(!f.isValid());

  // Always invalid without a type
  f.data = f.data.fill('\x23', 1);
  QVERIFY(!f.isValid());

  // SHA1 Tests
  f.type = Fingerprint::Type::SHA1;

  // Invalid SHA1, no Data
  f.data.clear();
  QVERIFY(!f.isValid());

  // Invalid SHA1, 5 bytes
  f.data = f.data.fill('\x23', 5);
  QVERIFY(!f.isValid());

  // Valid SHA1, 20 bytes
  f.data = f.data.fill('\x23', 20);
  QVERIFY(f.isValid());

  // Invalid SHA1, 25 bytes
  f.data = f.data.fill('\x23', 25);
  QVERIFY(!f.isValid());

  // SHA256 Tests
  f.type = Fingerprint::Type::SHA256;

  // Invalid SHA256, no Data
  f.data.clear();
  QVERIFY(!f.isValid());

  // Invalid SHA256, 16 bytes
  f.data = f.data.fill('\x23', 16);
  QVERIFY(!f.isValid());

  // Valid SHA256, 32 bytes
  f.data = f.data.fill('\x23', 32);
  QVERIFY(f.isValid());

  // Invalid SHA256, 50 bytes
  f.data = f.data.fill('\x23', 50);
  QVERIFY(!f.isValid());
}

void FingerprintTests::test_toDbLine()
{
  Fingerprint f;

  // Invalid Fingerprints return empty string
  QVERIFY(f.toDbLine().isEmpty());

  // Always invalid without a type
  f.data = f.data.fill('\x23', 20);
  QVERIFY(f.toDbLine().isEmpty());

  // Invalid SHA1, type w/o data
  f.type = Fingerprint::Type::SHA1;
  f.data.clear();
  QVERIFY(f.toDbLine().isEmpty());

  // Valid Sha1
  f.data = f.data.fill('\x23', 20);
  auto expectedString = QStringLiteral("v2:sha1:2323232323232323232323232323232323232323");
  QCOMPARE(f.toDbLine(), expectedString);

  // Valid Sha256
  f.type = Fingerprint::Type::SHA256;
  f.data = f.data.fill('\x23', 32);
  expectedString = QStringLiteral("v2:sha256:2323232323232323232323232323232323232323232323232323232323232323");
  QCOMPARE(f.toDbLine(), expectedString);
}

void FingerprintTests::test_fromDbLine()
{
  Fingerprint expected;

  // The Following are all invalid
  auto actual = Fingerprint::fromDbLine("");
  QCOMPARE(actual, expected);

  actual = Fingerprint::fromDbLine("abcd");
  QCOMPARE(actual, expected);

  actual = Fingerprint::fromDbLine("v1:algo:something");
  QCOMPARE(actual, expected);

  actual = Fingerprint::fromDbLine("v2:algo:something");
  expected.data = QByteArray::fromHex(QString("something").toLatin1());
  QCOMPARE(actual, expected);

  actual = Fingerprint::fromDbLine("v2:algo:01020304abc");
  expected.data = QByteArray::fromHex(QString("01020304abc").toLatin1());
  QCOMPARE(actual, expected);

  actual = Fingerprint::fromDbLine("v2:algo:01020304ZZ");
  expected.data = QByteArray::fromHex(QString("01020304ZZ").toLatin1());
  QCOMPARE(actual, expected);

  // Test V1 Only support Sha1
  expected.type = Fingerprint::Type::SHA1;
  expected.data =
      QByteArray::fromRawData("\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23", 20);
  actual = Fingerprint::fromDbLine("23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23");
  QCOMPARE(actual, expected);

  // V1 does not support SHA256
  expected.type = Fingerprint::Type::SHA256;
  expected.data = QByteArray::fromRawData(
      "\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23"
      "\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23",
      32
  );
  actual = Fingerprint::fromDbLine(
      "23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23:23"
  );
  QCOMPARE_NE(actual, expected);

  // V2 SHA1 Test
  expected.type = Fingerprint::Type::SHA1;
  expected.data =
      QByteArray::fromRawData("\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23", 20);
  actual = Fingerprint::fromDbLine("v2:sha1:2323232323232323232323232323232323232323");
  QCOMPARE(actual, expected);

  // V2 SHA1 Invalid Input
  actual = Fingerprint::fromDbLine("v2:sha1:23232323232323232323232323232323232323");
  QCOMPARE_NE(actual, expected);

  // V2 SHA256 Test
  expected.type = Fingerprint::Type::SHA256;
  expected.data = QByteArray::fromRawData(
      "\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23"
      "\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23\x23",
      32
  );
  actual = Fingerprint::fromDbLine("v2:sha256:2323232323232323232323232323232323232323232323232323232323232323");
  QCOMPARE(actual, expected);

  // V2 SHA256 Invalid Input
  actual = Fingerprint::fromDbLine("v2:sha256:232323232323232323232323232323232323232323232323232323");
  QCOMPARE_NE(actual, expected);
}

void FingerprintTests::test_typeToString()
{
  Fingerprint expected;

  expected.type = Fingerprint::Type::Invalid;
  QCOMPARE(expected.type, Fingerprint::Type::Invalid);
  QCOMPARE(Fingerprint::typeToString(expected.type), QStringLiteral("invalid"));

  expected.type = Fingerprint::Type::SHA1;
  QCOMPARE(expected.type, Fingerprint::Type::SHA1);
  QCOMPARE(Fingerprint::typeToString(expected.type), QStringLiteral("sha1"));

  expected.type = Fingerprint::Type::SHA256;
  QCOMPARE(expected.type, Fingerprint::Type::SHA256);
  QCOMPARE(Fingerprint::typeToString(expected.type), QStringLiteral("sha256"));
}

void FingerprintTests::test_typeFromString()
{
  QCOMPARE(Fingerprint::Type::SHA1, Fingerprint::typeFromString("sha1"));
  QCOMPARE(Fingerprint::Type::SHA1, Fingerprint::typeFromString("SHA1"));
  QCOMPARE(Fingerprint::Type::SHA256, Fingerprint::typeFromString("sha256"));
  QCOMPARE(Fingerprint::Type::SHA256, Fingerprint::typeFromString("SHA256"));

  QCOMPARE(Fingerprint::Type::Invalid, Fingerprint::typeFromString("invalid"));
  QCOMPARE(Fingerprint::Type::Invalid, Fingerprint::typeFromString(""));
  QCOMPARE(Fingerprint::Type::Invalid, Fingerprint::typeFromString("230p89jivon345"));
}

QTEST_MAIN(FingerprintTests)
