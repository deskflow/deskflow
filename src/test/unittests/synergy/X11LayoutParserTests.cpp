/*
 * synergy -- mouse and keyboard sharing utility
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

#include "synergy/X11LayoutsParser.h"
#include "test/global/gtest.h"
#include <fstream>

void createTestFiles()
{
    std::ofstream correctKeyboardFile ("correctKeyboard");
    if(!correctKeyboardFile.is_open()) {
        FAIL();
    }

    correctKeyboardFile << "XKBLAYOUT=us,ru" << std::endl;
    correctKeyboardFile << "XKBVARIANT=eng," << std::endl;
    correctKeyboardFile << "BACKSPACE=guess" << std::endl;
    correctKeyboardFile.close();

    std::ofstream incorrectKeyboardFile1 ("incorrectKeyboard1");
    if(!incorrectKeyboardFile1.is_open()) {
        FAIL();
    }

    incorrectKeyboardFile1 << "XKBLAYOUT=unknownLangCode" << std::endl;
    incorrectKeyboardFile1 << "XKBVARIANT=eng" << std::endl;
    incorrectKeyboardFile1 << "BACKSPACE=guess" << std::endl;
    incorrectKeyboardFile1.close();

    std::ofstream incorrectKeyboardFile2 ("incorrectKeyboard2");
    if(!incorrectKeyboardFile2.is_open()) {
        FAIL();
    }

    incorrectKeyboardFile2 << "XKBLAYOUT=us" << std::endl;
    incorrectKeyboardFile2 << "XKBVARIANT=unknownLangVariant" << std::endl;
    incorrectKeyboardFile2 << "BACKSPACE=guess" << std::endl;
    incorrectKeyboardFile2.close();

    std::ofstream correctEvdevFile ("correctEvdev.xml");
    if(!correctEvdevFile.is_open()) {
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

    std::ofstream incorrectEvdevFile1 ("incorrectEvdev1.xml");
    if(!incorrectEvdevFile1.is_open()) {
        FAIL();
    }

    incorrectEvdevFile1 << "<incorrectRootTag></incorrectRootTag>" << std::endl;
    incorrectEvdevFile1.close();

    std::ofstream incorrectEvdevFile2 ("incorrectEvdev2.xml");
    if(!incorrectEvdevFile2.is_open()) {
        FAIL();
    }

    incorrectEvdevFile2 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
    incorrectEvdevFile2 << "<xkbConfigRegistry version=\"1.1\">" << std::endl;
    incorrectEvdevFile2 << "</xkbConfigRegistry>" << std::endl;
    incorrectEvdevFile2.close();

    std::ofstream incorrectEvdevFile3 ("incorrectEvdev3.xml");
    if(!incorrectEvdevFile3.is_open()) {
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
    std::vector<String> expectedResult = { "en", "ru" };
    auto parsedResult = X11LayoutsParser::getX11LanguageList("correctKeyboard", "correctEvdev.xml");

    EXPECT_TRUE(parsedResult == expectedResult);
}

TEST(X11LayoutsParsingTests, xmlParsingMissedKeyboardFileTest)
{
    auto parsedResult = X11LayoutsParser::getX11LanguageList("missedFile", "correctEvdev.xml");
    EXPECT_TRUE(parsedResult.empty());
}

TEST(X11LayoutsParsingTests, xmlParsingMissedEvdevFileTest)
{
    auto parsedResult = X11LayoutsParser::getX11LanguageList("correctKeyboard", "missedFile");
    EXPECT_TRUE(parsedResult.empty());
}

TEST(X11LayoutsParsingTests, xmlParsingIncorrectEvdevFileTest)
{
    std::vector<String> parsedResult;
    parsedResult = X11LayoutsParser::getX11LanguageList("correctKeyboard", "incorrectEvdev1.xml");
    EXPECT_TRUE(parsedResult.empty());
    parsedResult = X11LayoutsParser::getX11LanguageList("correctKeyboard", "incorrectEvdev2.xml");
    EXPECT_TRUE(parsedResult.empty());
    parsedResult = X11LayoutsParser::getX11LanguageList("correctKeyboard", "incorrectEvdev3.xml");
    EXPECT_TRUE(parsedResult.empty());
}

TEST(X11LayoutsParsingTests, xmlParsingIncorrectKeyboardFileTest)
{
    std::vector<String> parsedResult;
    parsedResult = X11LayoutsParser::getX11LanguageList("incorrectKeyboard1", "correctEvdev.xml");
    EXPECT_TRUE(parsedResult.empty());
    parsedResult = X11LayoutsParser::getX11LanguageList("incorrectKeyboard2", "correctEvdev.xml");
    EXPECT_TRUE(parsedResult.empty());
}
