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

#include <QDomDocument>
#include <QFile>

#include "DeskflowXkbKeyboard.h"
#include "ISO639Table.h"
#include "X11LayoutsParser.h"
#include "base/Log.h"

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

bool X11LayoutsParser::readXMLConfigItemElem(const QDomNode &node, std::vector<Lang> &langList)
{
  auto configItemElem = node.firstChildElement("configItem");
  if (configItemElem.isNull()) {
    LOG_WARN("failed to read \"configItem\" in xml file");
    return false;
  }

  langList.emplace_back();
  if (auto nameElem = configItemElem.firstChildElement("name"); !nameElem.isNull())
    langList.back().name = nameElem.toElement().text().toStdString();

  if (auto languageListElem = configItemElem.elementsByTagName("languageList"); !languageListElem.isEmpty()) {
    for (int i = 0; i < languageListElem.count(); i++) {
      const auto isoElem = languageListElem.at(i).namedItem("iso639Id").toElement();
      langList.back().layoutBaseISO639_2.emplace_back(isoElem.text().toStdString());
    }
  }

  return true;
}

std::vector<X11LayoutsParser::Lang> X11LayoutsParser::getAllLanguageData(const std::string &pathToEvdevFile)
{
  std::vector<Lang> allCodes;

  QFile inFile(QString::fromStdString(pathToEvdevFile));
  if (!inFile.open(QIODevice::ReadOnly)) {
    LOG_WARN("unable to open %s", pathToEvdevFile.c_str());
    return allCodes;
  }

  QDomDocument xmlDoc;
  xmlDoc.setContent(inFile.readAll());

  const auto xkbConfigElem = xmlDoc.firstChildElement("xkbConfigRegistry");
  if (xkbConfigElem.isNull()) {
    LOG_WARN("failed to read xkbConfigRegistry in %s", pathToEvdevFile.c_str());
    return allCodes;
  }

  auto layoutListElem = xkbConfigElem.firstChildElement("layoutList");
  if (layoutListElem.isNull()) {
    LOG_WARN("failed to read layoutList in %s", pathToEvdevFile.c_str());
    return allCodes;
  }

  const auto layouts = layoutListElem.elementsByTagName("layout");
  for (int i = 0; i < layouts.count(); i++) {
    auto item = layouts.at(i);
    if (!readXMLConfigItemElem(item, allCodes))
      continue;

    auto variantListElem = item.namedItem("variantList").childNodes();
    for (int j = 0; j < variantListElem.count(); j++)
      readXMLConfigItemElem(variantListElem.at(j), allCodes.back().variants);
  }
  return allCodes;
}

void X11LayoutsParser::appendVectorUniq(const std::vector<std::string> &source, std::vector<std::string> &dst)
{
  for (const auto &elem : source) {
    if (std::ranges::find_if(dst, [&elem](const std::string_view &s) { return s == elem; }) == dst.end()) {
      dst.push_back(elem);
    }
  }
}

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
    if (layoutNames[i].empty()) {
      LOG_DEBUG("skip converting empty layout name");
      continue;
    }

    auto langIter = std::ranges::find_if(allLang, [&layoutName](const Lang &l) { return l.name == layoutName; });
    if (langIter == allLang.end()) {
      LOG_WARN("language \"%s\" is unknown", layoutNames[i].c_str());
      continue;
    }

    const std::vector<std::string> *toCopy = nullptr;
    if (i < layoutVariantNames.size()) {
      if (layoutVariantNames[i].empty()) {
        toCopy = &langIter->layoutBaseISO639_2;
      } else {
        const auto &variantName = layoutVariantNames[i];
        auto langVariantIter =
            std::ranges::find_if(langIter->variants, [&variantName](const Lang &l) { return l.name == variantName; });
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

std::string X11LayoutsParser::convertLayoutToISO(
    const std::string &pathToEvdevFile, const std::string &layoutLangCode, bool needToReloadFiles
)
{
  if (layoutLangCode.empty()) {
    LOG_DEBUG1("skip converting empty layout lang code");
    return "";
  }

  std::vector<std::string> iso639_2Codes;
  convertLayoutToISO639_2(pathToEvdevFile, needToReloadFiles, {layoutLangCode}, {""}, iso639_2Codes);
  if (iso639_2Codes.empty()) {
    LOG_WARN("failed to convert layout lang code: \"%s\"", layoutLangCode.c_str());
    return "";
  }

  auto iso639_1Codes = convertISO639_2ToISO639_1(iso639_2Codes);
  if (iso639_1Codes.empty()) {
    LOG_WARN("failed to convert ISO639/2 lang code to ISO639/1");
    return "";
  }

  return *iso639_1Codes.begin();
}

std::vector<std::string> X11LayoutsParser::convertISO639_2ToISO639_1(const std::vector<std::string> &iso639_2Codes)
{
  std::vector<std::string> result;
  for (const auto &isoCode : iso639_2Codes) {
    const auto &tableIter = std::ranges::find_if(ISO_Table, [&isoCode](const std::pair<std::string, std::string> &c) {
      return c.first == isoCode;
    });
    if (tableIter == ISO_Table.end()) {
      LOG_WARN("the ISO 639-2 code \"%s\" is missed in table", isoCode.c_str());
      continue;
    }

    appendVectorUniq({tableIter->second}, result);
  }

  return result;
}

#endif // WINAPI_XWINDOWS
