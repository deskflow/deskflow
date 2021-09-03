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

TEST(X11LayoutsParsingTests, xmlParsingTest)
{
    std::vector<String> expectedResult = {
        "en",
        "ru"
    };

    {
        std::ofstream outfile ("testKeyboardFile");
        outfile << "XKBLAYOUT=us,ru" << std::endl;
        outfile << "XKBVARIANT=," << std::endl;
        outfile << "BACKSPACE=guess" << std::endl;
    }

    {
        std::ofstream outfile ("testEvdev.xml");
        outfile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
        outfile << "<xkbConfigRegistry version=\"1.1\">" << std::endl;
        outfile << "  <layoutList>" << std::endl;
        outfile << "    <layout>" << std::endl;
        outfile << "      <configItem>" << std::endl;
        outfile << "        <name>us</name>" << std::endl;
        outfile << "        <!-- Keyboard indicator for English layouts -->" << std::endl;
        outfile << "        <shortDescription>en</shortDescription>" << std::endl;
        outfile << "        <description>English (US)</description>" << std::endl;
        outfile << "        <languageList>" << std::endl;
        outfile << "          <iso639Id>eng</iso639Id>" << std::endl;
        outfile << "        </languageList>" << std::endl;
        outfile << "      </configItem>" << std::endl;
        outfile << "      <variantList>" << std::endl;
        outfile << "        <variant>" << std::endl;
        outfile << "          <configItem>" << std::endl;
        outfile << "            <name>chr</name>" << std::endl;
        outfile << "            <!-- Keyboard indicator for Cherokee layouts -->" << std::endl;
        outfile << "            <shortDescription>chr</shortDescription>" << std::endl;
        outfile << "            <description>Cherokee</description>" << std::endl;
        outfile << "            <languageList>" << std::endl;
        outfile << "              <iso639Id>chr</iso639Id>" << std::endl;
        outfile << "            </languageList>" << std::endl;
        outfile << "          </configItem>" << std::endl;
        outfile << "        </variant>" << std::endl;
        outfile << "      </variantList>" << std::endl;
        outfile << "    </layout>" << std::endl;
        outfile << "    <layout>" << std::endl;
        outfile << "      <configItem>" << std::endl;
        outfile << "        <name>ru</name>" << std::endl;
        outfile << "        <!-- Keyboard indicator for Russian layouts -->" << std::endl;
        outfile << "        <shortDescription>ru</shortDescription>" << std::endl;
        outfile << "        <description>Russian</description>" << std::endl;
        outfile << "        <languageList>" << std::endl;
        outfile << "          <iso639Id>rus</iso639Id>" << std::endl;
        outfile << "        </languageList>" << std::endl;
        outfile << "      </configItem>" << std::endl;
        outfile << "    </layout>" << std::endl;
        outfile << "  </layoutList>" << std::endl;
        outfile << "</xkbConfigRegistry>" << std::endl;
    }

    auto parsedResult = X11LayoutsParser::getX11LanguageList("testKeyboardFile", "testEvdev.xml");

    EXPECT_TRUE(parsedResult == expectedResult);
}
