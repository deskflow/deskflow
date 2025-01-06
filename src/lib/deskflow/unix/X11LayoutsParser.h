/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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
#pragma once

#include <string>
#include <vector>

namespace pugi {
class xml_node;
}

class X11LayoutsParser
{
public:
  static std::vector<std::string> getX11LanguageList(const std::string &pathToEvdevFile);
  static std::string convertLayotToISO(
      const std::string &pathToEvdevFile, const std::string &layoutLangCode, bool needToReloadFiles = false
  );

private:
  struct Lang
  {
    std::string name = "";
    std::vector<std::string> layoutBaseISO639_2;
    std::vector<Lang> variants;
  };

  static bool readXMLConfigItemElem(const pugi::xml_node *root, std::vector<Lang> &langList);

  static std::vector<Lang> getAllLanguageData(const std::string &pathToEvdevFile);

  static void appendVectorUniq(const std::vector<std::string> &source, std::vector<std::string> &dst);

  static void convertLayoutToISO639_2(
      const std::string &pathToEvdevFile, bool needToReloadEvdev, const std::vector<std::string> &layoutNames,
      const std::vector<std::string> &layoutVariantNames, std::vector<std::string> &iso639_2Codes
  );

  static std::vector<std::string> convertISO639_2ToISO639_1(const std::vector<std::string> &iso639_2Codes);
};

#endif // WINAPI_XWINDOWS
