/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
