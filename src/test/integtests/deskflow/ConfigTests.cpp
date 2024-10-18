/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
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
