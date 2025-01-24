/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#if WINAPI_XWINDOWS
#include <algorithm>
#include <fstream>
#include <sstream>

#include "DeskflowXkbKeyboard.h"
#include "ISO639Table.h"
#include "X11LayoutsParser.h"
#include "base/Log.h"
#include "pugixml.hpp"

namespace {

void splitLine(std::vector<std::string> &parts, const std::string &line, char delimiter)
{
  std::stringstream stream(line);
  while (stream.good()) {
    std::string part;
    getline(stream, part, delimiter);
    parts.push_back(part);
  }
}

} // namespace

bool X11LayoutsParser::readXMLConfigItemElem(const pugi::xml_node *root, std::vector<Lang> &langList)
{
  auto configItemElem = root->child("configItem");
  if (!configItemElem) {
    LOG((CLOG_WARN "failed to read \"configItem\" in xml file"));
    return false;
  }

  langList.emplace_back();
  auto nameElem = configItemElem.child("name");
  if (nameElem) {
    langList.back().name = nameElem.text().as_string();
  }

  auto languageListElem = configItemElem.child("languageList");
  if (languageListElem) {
    for (pugi::xml_node isoElem : languageListElem.children("iso639Id")) {
      langList.back().layoutBaseISO639_2.emplace_back(isoElem.text().as_string());
    }
  }

  return true;
}

std::vector<X11LayoutsParser::Lang> X11LayoutsParser::getAllLanguageData(const std::string &pathToEvdevFile)
{
  std::vector<Lang> allCodes;
  pugi::xml_document doc;
  if (!doc.load_file(pathToEvdevFile.c_str())) {
    LOG((CLOG_WARN "failed to open %s", pathToEvdevFile.c_str()));
    return allCodes;
  }

  auto xkbConfigElem = doc.child("xkbConfigRegistry");
  if (!xkbConfigElem) {
    LOG((CLOG_WARN "failed to read xkbConfigRegistry in %s", pathToEvdevFile.c_str()));
    return allCodes;
  }

  auto layoutListElem = xkbConfigElem.child("layoutList");
  if (!layoutListElem) {
    LOG((CLOG_WARN "failed to read layoutList in %s", pathToEvdevFile.c_str()));
    return allCodes;
  }

  for (pugi::xml_node layoutElem : layoutListElem.children("layout")) {
    if (!readXMLConfigItemElem(&layoutElem, allCodes)) {
      continue;
    }

    auto variantListElem = layoutElem.child("variantList");
    if (variantListElem) {
      for (pugi::xml_node variantElem : variantListElem.children("variant")) {
        readXMLConfigItemElem(&variantElem, allCodes.back().variants);
      }
    }
  }

  return allCodes;
}

void X11LayoutsParser::appendVectorUniq(const std::vector<std::string> &source, std::vector<std::string> &dst)
{
  for (const auto &elem : source) {
    if (std::find_if(dst.begin(), dst.end(), [elem](const std::string &s) { return s == elem; }) == dst.end()) {
      dst.push_back(elem);
    }
  }
};

void X11LayoutsParser::convertLayoutToISO639_2(
    const std::string &pathToEvdevFile, bool needToReloadEvdev, const std::vector<std::string> &layoutNames,
    const std::vector<std::string> &layoutVariantNames, std::vector<std::string> &iso639_2Codes
)
{
  static std::vector<X11LayoutsParser::Lang> allLang;
  if (allLang.empty() || needToReloadEvdev) {
    allLang = getAllLanguageData(pathToEvdevFile);
  }
  for (size_t i = 0; i < layoutNames.size(); i++) {
    const auto &layoutName = layoutNames[i];
    auto langIter =
        std::find_if(allLang.begin(), allLang.end(), [&layoutName](const Lang &l) { return l.name == layoutName; });
    if (langIter == allLang.end()) {
      LOG((CLOG_WARN "language \"%s\" is unknown", layoutNames[i].c_str()));
      continue;
    }

    const std::vector<std::string> *toCopy = nullptr;
    if (i < layoutVariantNames.size()) {
      if (layoutVariantNames[i].empty()) {
        toCopy = &langIter->layoutBaseISO639_2;
      } else {
        const auto &variantName = layoutVariantNames[i];
        auto langVariantIter =
            std::find_if(langIter->variants.begin(), langIter->variants.end(), [&variantName](const Lang &l) {
              return l.name == variantName;
            });
        if (langVariantIter == langIter->variants.end()) {
          LOG(
              (CLOG_WARN "variant \"%s\" of language \"%s\" is unknown", layoutVariantNames[i].c_str(),
               layoutNames[i].c_str())
          );
          continue;
        }

        if (langVariantIter->layoutBaseISO639_2.empty()) {
          toCopy = &langIter->layoutBaseISO639_2;
        } else {
          toCopy = &langVariantIter->layoutBaseISO639_2;
        }
      }
    } else {
      toCopy = &langIter->layoutBaseISO639_2;
    }

    if (toCopy) {
      appendVectorUniq(*toCopy, iso639_2Codes);
    }
  }
}

std::vector<std::string> X11LayoutsParser::getX11LanguageList(const std::string &pathToEvdevFile)
{
  std::vector<std::string> layoutNames;
  std::vector<std::string> layoutVariantNames;

  deskflow::linux::DeskflowXkbKeyboard keyboard;
  splitLine(layoutNames, keyboard.getLayout(), ',');
  splitLine(layoutVariantNames, keyboard.getVariant(), ',');

  std::vector<std::string> iso639_2Codes;
  iso639_2Codes.reserve(layoutNames.size());
  convertLayoutToISO639_2(pathToEvdevFile, true, layoutNames, layoutVariantNames, iso639_2Codes);
  return convertISO639_2ToISO639_1(iso639_2Codes);
}

std::string X11LayoutsParser::convertLayotToISO(
    const std::string &pathToEvdevFile, const std::string &layoutLangCode, bool needToReloadFiles
)
{
  std::vector<std::string> iso639_2Codes;
  convertLayoutToISO639_2(pathToEvdevFile, needToReloadFiles, {layoutLangCode}, {""}, iso639_2Codes);
  if (iso639_2Codes.empty()) {
    LOG((CLOG_WARN "failed to convert layout lang code: \"%s\"", layoutLangCode.c_str()));
    return "";
  }

  auto iso639_1Codes = convertISO639_2ToISO639_1(iso639_2Codes);
  if (iso639_1Codes.empty()) {
    LOG((CLOG_WARN "failed to convert ISO639/2 lang code to ISO639/1"));
    return "";
  }

  return *iso639_1Codes.begin();
}

std::vector<std::string> X11LayoutsParser::convertISO639_2ToISO639_1(const std::vector<std::string> &iso639_2Codes)
{
  std::vector<std::string> result;
  for (const auto &isoCode : iso639_2Codes) {
    const auto &tableIter =
        std::find_if(ISO_Table.begin(), ISO_Table.end(), [&isoCode](const std::pair<std::string, std::string> &c) {
          return c.first == isoCode;
        });
    if (tableIter == ISO_Table.end()) {
      LOG((CLOG_WARN "the ISO 639-2 code \"%s\" is missed in table", isoCode.c_str()));
      continue;
    }

    appendVectorUniq({tableIter->second}, result);
  }

  return result;
}

#endif // WINAPI_XWINDOWS
