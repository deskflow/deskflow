/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/Config.h"

#include <fstream>
#include <gtest/gtest.h>

using namespace deskflow;

const auto kTestFilename = "tmp/test/test.toml";

TEST(ConfigTests, load_fileExists_loadsConfig)
{
  std::ofstream testFile(kTestFilename);
  testFile << "[test.args]\n"
              R"(test-arg = "test opt")";
  testFile.close();
  Config config(kTestFilename, "test");

  const auto result = config.load("test");

  ASSERT_TRUE(result);
  ASSERT_EQ(config.argc(), 3);
  ASSERT_STREQ(config.argv()[0], "test");
  ASSERT_STREQ(config.argv()[1], "--test-arg");
  ASSERT_STREQ(config.argv()[2], "test opt");
}

TEST(ConfigTests, load_filenameEmpty_throwsException)
{
  EXPECT_THROW(
      {
        Config config("", "test");

        config.load("test");
      },
      Config::NoConfigFilenameError
  );
}

TEST(ConfigTests, load_fileDoesNotExist_returnsFalse)
{
  Config config("nonexistent.toml", "test");

  const auto result = config.load("test");

  ASSERT_FALSE(result);
}

TEST(ConfigTests, load_invalidConfig_throwsException)
{
  EXPECT_THROW(
      {
        std::ofstream testFile(kTestFilename);
        testFile << "foobar";
        testFile.close();

        Config config(kTestFilename, "test");

        config.load("test");
      },
      Config::ParseError
  );
}

TEST(ConfigTests, load_sectionMissing_returnsFalse)
{
  std::ofstream testFile(kTestFilename);
  testFile.close();
  Config config(kTestFilename, "missing");

  const auto result = config.load("test");

  ASSERT_FALSE(result);
}

TEST(ConfigTests, load_notTable_returnsFalse)
{
  std::ofstream testFile(kTestFilename);
  testFile << "[test]";
  testFile.close();
  Config config(kTestFilename, "test");

  const auto result = config.load("test");

  ASSERT_FALSE(result);
}

TEST(ConfigTests, load_lastArg_returnsLast)
{
  std::ofstream testFile(kTestFilename);
  testFile << "[test.args]\n"
              R"(_last = "test last")"
              "\n"
              R"(test-second = true)";
  testFile.close();
  Config config(kTestFilename, "test");

  const auto result = config.load("test-first");

  ASSERT_TRUE(result);
  ASSERT_EQ(config.argc(), 3);
  ASSERT_STREQ(config.argv()[0], "test-first");
  ASSERT_STREQ(config.argv()[1], "--test-second");
  ASSERT_STREQ(config.argv()[2], "test last");
}

TEST(ConfigTests, load_noArgs_returnsFalse)
{
  std::ofstream testFile(kTestFilename);
  testFile << "[test.args]";
  testFile.close();
  Config config(kTestFilename, "test");

  const auto result = config.load("");

  ASSERT_FALSE(result);
}
