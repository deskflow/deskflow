/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Path.h"
#include <fstream>

#include <gtest/gtest.h>

const std::string testDir = "tmp/test";
const std::wstring testDirW = L"tmp/test";

TEST(PathTests, open_file_using_path)
{
  std::string utf8FileName = testDir + "/тіás.txt";
#if SYSAPI_WIN32
  // Windows uses UTF-16 for file path and names
  std::wstring fileName = testDirW + L"/\x0442\x0456\x00E1\x0073\x002E\x0074\x0078\x0074";
#else
  std::string fileName = utf8FileName;
#endif

  std::fstream file(fileName, std::fstream::out);
  file << "test";
  file.close();

  std::ifstream inFile(deskflow::filesystem::path(utf8FileName));
  EXPECT_TRUE(inFile.is_open());
}
