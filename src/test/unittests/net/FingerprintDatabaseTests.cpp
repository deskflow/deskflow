/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/FingerprintDatabase.h"

#include <gtest/gtest.h>

namespace deskflow {

TEST(FingerprintDatabase, parseDbLine)
{
  ASSERT_FALSE(FingerprintDatabase::parseDbLine("").valid());
  ASSERT_FALSE(FingerprintDatabase::parseDbLine("abcd").valid());
  ASSERT_FALSE(FingerprintDatabase::parseDbLine("v1:algo:something").valid());
  ASSERT_FALSE(FingerprintDatabase::parseDbLine("v2:algo:something").valid());
  ASSERT_FALSE(FingerprintDatabase::parseDbLine("v2:algo:01020304abc").valid());
  ASSERT_FALSE(FingerprintDatabase::parseDbLine("v2:algo:01020304ZZ").valid());
  ASSERT_EQ(FingerprintDatabase::parseDbLine("v2:algo:01020304ab"), (FingerprintData{"algo", {1, 2, 3, 4, 0xab}}));
}

TEST(FingerprintDatabase, read)
{
  std::istringstream stream;
  stream.str(R"(
v2:algo1:01020304ab
v2:algo2:03040506ab
AB:CD:EF:00:01:02:03:04:05:06:07:08:09:10:11:12:13:14:15:16
)");
  FingerprintDatabase db;
  db.readStream(stream);

  std::vector<FingerprintData> expected = {
      {"algo1", {1, 2, 3, 4, 0xab}},
      {"algo2", {3, 4, 5, 6, 0xab}},
      {"sha1", {0xab, 0xcd, 0xef, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16}},
  };
  ASSERT_EQ(db.fingerprints(), expected);
}

TEST(FingerprintDatabase, write)
{
  std::ostringstream stream;

  FingerprintDatabase db;
  db.addTrusted({"algo1", {1, 2, 3, 4, 0xab}});
  db.addTrusted({"algo2", {3, 4, 5, 6, 0xab}});
  db.writeStream(stream);

  ASSERT_EQ(stream.str(), R"(v2:algo1:01020304ab
v2:algo2:03040506ab
)");
}

TEST(FingerprintDatabase, clear)
{
  FingerprintDatabase db;
  db.addTrusted({"algo1", {1, 2, 3, 4, 0xab}});
  db.clear();
  ASSERT_TRUE(db.fingerprints().empty());
}

TEST(FingerprintDatabase, addTrusted_noDuplicates)
{
  FingerprintDatabase db;
  db.addTrusted({"algo1", {1, 2, 3, 4, 0xab}});
  db.addTrusted({"algo2", {3, 4, 5, 6, 0xab}});
  db.addTrusted({"algo1", {1, 2, 3, 4, 0xab}});
  ASSERT_EQ(db.fingerprints().size(), 2);
}

TEST(FingerprintDatabase, isTrusted)
{
  FingerprintDatabase db;
  db.addTrusted({"algo1", {1, 2, 3, 4, 0xab}});
  ASSERT_TRUE(db.isTrusted({"algo1", {1, 2, 3, 4, 0xab}}));
  ASSERT_FALSE(db.isTrusted({"algo2", {1, 2, 3, 4, 0xab}}));
  ASSERT_FALSE(db.isTrusted({"algo1", {1, 2, 3, 4, 0xac}}));
}

} // namespace deskflow
