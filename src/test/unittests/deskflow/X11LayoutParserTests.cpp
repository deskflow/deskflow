/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

#if WINAPI_XWINDOWS
#include "deskflow/unix/X11LayoutsParser.h"

#include <fstream>
#include <gtest/gtest.h>

const std::string testDir = "tmp/test";

void createTestFiles()
{
  std::ofstream correctEvdevFile(testDir + "/correctEvdev.xml");
  if (!correctEvdevFile.is_open()) {
    FAIL();
  }

  correctEvdevFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
  correctEvdevFile << "<xkbConfigRegistry version=\"1.1\">" << std::endl;
  correctEvdevFile << "  <layoutList>" << std::endl;
  correctEvdevFile << "    <layout>" << std::endl;
  correctEvdevFile << "      <configItem>" << std::endl;
  correctEvdevFile << "        <name>us</name>" << std::endl;
  correctEvdevFile << "        <!-- Keyboard indicator for English layouts -->" << std::endl;
  correctEvdevFile << "        <shortDescription>en</shortDescription>" << std::endl;
  correctEvdevFile << "        <description>English (US)</description>" << std::endl;
  correctEvdevFile << "        <languageList>" << std::endl;
  correctEvdevFile << "          <iso639Id>eng</iso639Id>" << std::endl;
  correctEvdevFile << "        </languageList>" << std::endl;
  correctEvdevFile << "      </configItem>" << std::endl;
  correctEvdevFile << "      <variantList>" << std::endl;
  correctEvdevFile << "        <variant>" << std::endl;
  correctEvdevFile << "          <configItem>" << std::endl;
  correctEvdevFile << "            <name>eng</name>" << std::endl;
  correctEvdevFile << "            <shortDescription>eng</shortDescription>" << std::endl;
  correctEvdevFile << "            <description>Cherokee</description>" << std::endl;
  correctEvdevFile << "            <languageList>" << std::endl;
  correctEvdevFile << "              <iso639Id>eng</iso639Id>" << std::endl;
  correctEvdevFile << "            </languageList>" << std::endl;
  correctEvdevFile << "          </configItem>" << std::endl;
  correctEvdevFile << "        </variant>" << std::endl;
  correctEvdevFile << "      </variantList>" << std::endl;
  correctEvdevFile << "    </layout>" << std::endl;
  correctEvdevFile << "    <layout>" << std::endl;
  correctEvdevFile << "      <configItem>" << std::endl;
  correctEvdevFile << "        <name>ru</name>" << std::endl;
  correctEvdevFile << "        <!-- Keyboard indicator for Russian layouts -->" << std::endl;
  correctEvdevFile << "        <shortDescription>ru</shortDescription>" << std::endl;
  correctEvdevFile << "        <description>Russian</description>" << std::endl;
  correctEvdevFile << "        <languageList>" << std::endl;
  correctEvdevFile << "          <iso639Id>rus</iso639Id>" << std::endl;
  correctEvdevFile << "        </languageList>" << std::endl;
  correctEvdevFile << "      </configItem>" << std::endl;
  correctEvdevFile << "    </layout>" << std::endl;
  correctEvdevFile << "  </layoutList>" << std::endl;
  correctEvdevFile << "</xkbConfigRegistry>" << std::endl;
  correctEvdevFile.close();

  std::ofstream evdevFromFutureFile(testDir + "/evdevFromFuture.xml");
  if (!evdevFromFutureFile.is_open()) {
    FAIL();
  }

  evdevFromFutureFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
  evdevFromFutureFile << "<xkbConfigRegistry version=\"1.1\">" << std::endl;
  evdevFromFutureFile << "  <layoutList>" << std::endl;
  evdevFromFutureFile << "    <layout>" << std::endl;
  evdevFromFutureFile << "      <configItem>" << std::endl;
  evdevFromFutureFile << "        <name>futureLangName</name>" << std::endl;
  evdevFromFutureFile << "        <languageList>" << std::endl;
  evdevFromFutureFile << "          <iso639Id>fln</iso639Id>" << std::endl;
  evdevFromFutureFile << "        </languageList>" << std::endl;
  evdevFromFutureFile << "      </configItem>" << std::endl;
  evdevFromFutureFile << "    </layout>" << std::endl;
  evdevFromFutureFile << "  </layoutList>" << std::endl;
  evdevFromFutureFile << "</xkbConfigRegistry>" << std::endl;
  evdevFromFutureFile.close();

  std::ofstream incorrectEvdevFile1(testDir + "/incorrectEvdev1.xml");
  if (!incorrectEvdevFile1.is_open()) {
    FAIL();
  }

  incorrectEvdevFile1 << "<incorrectRootTag></incorrectRootTag>" << std::endl;
  incorrectEvdevFile1.close();

  std::ofstream incorrectEvdevFile2(testDir + "/incorrectEvdev2.xml");
  if (!incorrectEvdevFile2.is_open()) {
    FAIL();
  }

  incorrectEvdevFile2 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
  incorrectEvdevFile2 << "<xkbConfigRegistry version=\"1.1\">" << std::endl;
  incorrectEvdevFile2 << "</xkbConfigRegistry>" << std::endl;
  incorrectEvdevFile2.close();

  std::ofstream incorrectEvdevFile3(testDir + "/incorrectEvdev3.xml");
  if (!incorrectEvdevFile3.is_open()) {
    FAIL();
  }

  incorrectEvdevFile3 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
  incorrectEvdevFile3 << "<xkbConfigRegistry version=\"1.1\">" << std::endl;
  incorrectEvdevFile3 << "  <layoutList>" << std::endl;
  incorrectEvdevFile3 << "    <layout>" << std::endl;
  incorrectEvdevFile3 << "    </layout>" << std::endl;
  incorrectEvdevFile3 << "  </layoutList>" << std::endl;
  incorrectEvdevFile3 << "</xkbConfigRegistry>" << std::endl;
  incorrectEvdevFile3.close();
}

TEST(X11LayoutsParsingTests, xmlCorrectParsingTest)
{
  createTestFiles();
  std::vector<String> expectedResult = {"en", "ru"};
  auto parsedResult = X11LayoutsParser::getX11LanguageList(testDir + "/correctEvdev.xml");

  EXPECT_EQ(parsedResult, parsedResult);
}

TEST(X11LayoutsParsingTests, xmlParsingMissedEvdevFileTest)
{
  auto parsedResult = X11LayoutsParser::getX11LanguageList(testDir + "/missedFile");
  EXPECT_TRUE(parsedResult.empty());
}

TEST(X11LayoutsParsingTests, xmlParsingIncorrectEvdevFileTest)
{
  std::vector<String> parsedResult;
  parsedResult = X11LayoutsParser::getX11LanguageList(testDir + "/incorrectEvdev1.xml");
  EXPECT_TRUE(parsedResult.empty());
  parsedResult = X11LayoutsParser::getX11LanguageList(testDir + "/incorrectEvdev2.xml");
  EXPECT_TRUE(parsedResult.empty());
  parsedResult = X11LayoutsParser::getX11LanguageList(testDir + "/incorrectEvdev3.xml");
  EXPECT_TRUE(parsedResult.empty());
}

TEST(X11LayoutsParsingTests, layoutConvertTest)
{
  EXPECT_EQ(X11LayoutsParser::convertLayotToISO(testDir + "/correctEvdev.xml", "us", true), "en");
  EXPECT_EQ(X11LayoutsParser::convertLayotToISO(testDir + "/incorrectEvdev1.xml", "us", true), "");
  EXPECT_EQ(X11LayoutsParser::convertLayotToISO(testDir + "/evdevFromFuture.xml", "us", true), "");
}

#endif
