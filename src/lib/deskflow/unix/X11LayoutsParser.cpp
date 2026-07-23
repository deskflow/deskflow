/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#if WINAPI_XWINDOWS
#include <algorithm>
#include <sstream>

#include "base/Log.h"

#include "DeskflowXkbKeyboard.h"
#include "ISO639Table.h"
#include "X11LayoutsParser.h"

#include <xkbcommon/xkbregistry.h>

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

std::vector<X11LayoutsParser::Lang> X11LayoutsParser::getAllLanguageData()
{
  std::vector<Lang> allCodes;

  rxkb_context *ctx = rxkb_context_new(RXKB_CONTEXT_NO_FLAGS);

  if (!ctx) {
    LOG_WARN("failed to create xkb registry context");
    return allCodes;
  }

  if (!rxkb_context_parse_default_ruleset(ctx)) {
    LOG_WARN("failed to parse xkb registry ruleset");
    rxkb_context_unref(ctx);
    return allCodes;
  }

  for (rxkb_layout *layout = rxkb_layout_first((ctx)); layout; layout = rxkb_layout_next(layout)) {
    const char *name = rxkb_layout_get_name(layout);
    const char *variant = rxkb_layout_get_variant(layout);

    std::vector<std::string> isoCodes;
    for (rxkb_iso639_code *isoCode = rxkb_layout_get_iso639_first(layout); isoCode;
         isoCode = rxkb_iso639_code_next(isoCode)) {
      if (const char *code = rxkb_iso639_code_get_code(isoCode)) {
        isoCodes.emplace_back(code);
      }
    }

    if (!variant) {
      allCodes.emplace_back();
      allCodes.back().name = name ? name : "";
      allCodes.back().layoutBaseISO639_2 = std::move(isoCodes);
    } else {
      auto requiredName = name ? name : std::string{};
      auto iterator = std::ranges::find_if(allCodes, [&](const Lang &l) { return l.name == requiredName; });
      if (iterator == allCodes.end())
        continue;

      iterator->variants.emplace_back();
      iterator->variants.back().name = variant;
      iterator->variants.back().layoutBaseISO639_2 = std::move(isoCodes);
    }
  }

  rxkb_context_unref(ctx);
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
    const std::vector<std::string> &layoutNames, const std::vector<std::string> &layoutVariantNames,
    std::vector<std::string> &iso639_2Codes
)
{
  static std::vector<X11LayoutsParser::Lang> allLang;
  if (allLang.empty()) {
    allLang = getAllLanguageData();
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

std::vector<std::string> X11LayoutsParser::getX11LanguageList()
{
  std::vector<std::string> layoutNames;
  std::vector<std::string> layoutVariantNames;

  deskflow::linux::DeskflowXkbKeyboard keyboard;
  splitLine(layoutNames, keyboard.getLayout(), ',');
  splitLine(layoutVariantNames, keyboard.getVariant(), ',');

  std::vector<std::string> iso639_2Codes;
  iso639_2Codes.reserve(layoutNames.size());
  convertLayoutToISO639_2(layoutNames, layoutVariantNames, iso639_2Codes);
  return convertISO639_2ToISO639_1(iso639_2Codes);
}

std::string X11LayoutsParser::convertLayoutToISO(const std::string &layoutLangCode)
{
  if (layoutLangCode.empty()) {
    LOG_VERBOSE("skip converting empty layout lang code");
    return "";
  }

  std::vector<std::string> iso639_2Codes;
  convertLayoutToISO639_2({layoutLangCode}, {""}, iso639_2Codes);
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
