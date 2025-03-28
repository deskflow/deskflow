/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FingerprintDatabaseTests.h"

#include "net/Fingerprint.h"
#include "net/FingerprintDatabase.h"

#include <sstream>

void FingerprintDatabaseTests::readFile()
{
  QString data = R"(
v2:sha1:01020304ab
v2:sha1:03040506ab
AB:CD:EF:00:01:02:03:04:05:06:07:08:09:10:11:12:13:14:15:16
)";

  QTextStream stream(&data);

  FingerprintDatabase db;
  db.readStream(stream);

  // Only one will be in our list as only one is valid
  QList<Fingerprint> expected = {
      {Fingerprint::Type::SHA1,
       QByteArray::fromRawData("\xAB\xCD\xEF\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x10\x11\x12\x13\x14\x15\x16", 20)}
  };

  QCOMPARE(db.fingerprints(), expected);
}

void FingerprintDatabaseTests::writeFile()
{
  QString out;
  QTextStream stream(&out);

  FingerprintDatabase db;
  db.addTrusted(
      {Fingerprint::Type::SHA1, QByteArray::fromHex(QString("ABCDEF0001020304050607080910111213141516").toLatin1())}
  );
  db.addTrusted(
      {Fingerprint::Type::SHA1, QByteArray::fromHex(QString("0001020304050607080910111213141516ABCDEF").toLatin1())}
  );
  db.writeStream(stream);

  QCOMPARE(stream.readAll(), R"(v2:sha1:abcdef0001020304050607080910111213141516
v2:sha1:0001020304050607080910111213141516abcdef
)");
}

void FingerprintDatabaseTests::clear()
{
  FingerprintDatabase db;
  db.addTrusted({Fingerprint::Type::SHA1, QByteArray::fromHex(QString("01020304ab").toLatin1())});
  db.clear();

  QVERIFY(db.fingerprints().empty());
}

void FingerprintDatabaseTests::trusted()
{
  Fingerprint trusted1 = {Fingerprint::Type::SHA1, QByteArray::fromHex(QString("01020304ab").toLatin1())};
  Fingerprint trusted2 = {Fingerprint::Type::SHA1, QByteArray::fromHex(QString("03040506ab").toLatin1())};
  Fingerprint untrusted = {Fingerprint::Type::SHA1, QByteArray::fromHex(QString("01020304ac").toLatin1())};

  FingerprintDatabase db;

  db.addTrusted(trusted1);
  QVERIFY(db.isTrusted(trusted1));
  QVERIFY(!db.isTrusted(trusted2));
  QVERIFY(!db.isTrusted(untrusted));

  db.addTrusted(trusted2);
  QVERIFY(db.isTrusted(trusted1));
  QVERIFY(db.isTrusted(trusted2));
  QVERIFY(!db.isTrusted(untrusted));

  QCOMPARE(db.fingerprints().size(), 2);
}

QTEST_MAIN(FingerprintDatabaseTests)
