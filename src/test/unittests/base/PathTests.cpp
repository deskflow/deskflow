/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2021 Symless Ltd.
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

#include "base/Path.h"
#include <fstream>

#include "test/global/gtest.h"


TEST(PathTests, open_file_using_path)
{
   std::string utf8FileName = "тіás.txt";
#if SYSAPI_WIN32
   //Windows uses UTF-16 for file path and names
   std::wstring fileName = L"\x0442\x0456\x00E1\x0073\x002E\x0074\x0078\x0074";
#else
    std::string fileName = utf8FileName;
#endif

   std::fstream file(fileName, std::fstream::out);
   file << "test";
   file.close();

   std::ifstream inFile(synergy::filesystem::path(utf8FileName));
   EXPECT_TRUE(inFile.is_open());
}
