/*
 * synergy -- mouse and keyboard sharing utility
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

#if HAVE_TOMLPLUSPLUS

#include "synergy/Config.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace synergy;

const auto kTestFilename = "tmp/test/test.toml";

TEST(ConfigTests, LoadConfigFile) {
  std::ofstream testFile(kTestFilename);
  testFile << "[test.args]\n"
              R"(test-arg = "test opt")";
  testFile.close();

  try {
    Config config(kTestFilename, "test");

    ASSERT_TRUE(config.load("test"));
    ASSERT_EQ(config.argc(), 3);
    ASSERT_STREQ(config.argv()[0], "test");
    ASSERT_STREQ(config.argv()[1], "--test-arg");
    ASSERT_STREQ(config.argv()[2], "test opt");

  } catch (const std::exception &e) {
    FAIL() << e.what();
  }

  std::filesystem::remove(kTestFilename);
}

#endif // HAVE_TOMLPLUSPLUS
