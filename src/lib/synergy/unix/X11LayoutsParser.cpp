/*
 * synergy -- mouse and keyboard sharing utility
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

#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>

#include "base/Log.h"
#include "synergy/unix/X11LayoutsParser.h"
#include "ISO639Table.h"

bool
X11LayoutsParser::readAndTrimString(std::ifstream& file, String& line)
{
    auto result = (bool)std::getline(file, line);
    if(result) {
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](char ch) { return !std::isspace(ch); }));
    }
    return result;
}

std::vector<X11LayoutsParser::Lang>
X11LayoutsParser::getAllLanguageData()
{
    std::vector<Lang> allCodes;
    std::ifstream file("/usr/share/X11/xkb/rules/evdev.xml");
    if (!file.is_open()) {
        return allCodes;
    }

    String line;
    std::vector<std::pair<String, String*>> searchedTags = {std::make_pair("<name>", nullptr),
                                                                      std::make_pair("<shortDescription>", nullptr),
                                                                      std::make_pair("<description>", nullptr)};

    auto tryReadAllLanguageList = [&file, &line](std::vector<String>& result) {
        if(line != "<languageList>") {
            return;
        }

        String isoTag = "<iso639Id>";
        while (readAndTrimString(file, line) && line != "</languageList>") {
            auto strat = line.rfind(isoTag, 0);
            if (strat == 0) {
                result.push_back(line.substr(isoTag.size(), line.find("</", strat) - isoTag.size()));
            }
        }
    };

    std::function<void(std::vector<Lang>&)> readLangData = [&](std::vector<Lang>& langList) {
        if(line == "<configItem>") {
            langList.emplace_back(Lang());
            searchedTags[0].second = &langList.back().name;
            searchedTags[1].second = &langList.back().shortDescr;
            searchedTags[2].second = &langList.back().descr;
            while (readAndTrimString(file, line) && line != "</configItem>") {
                for(auto temp : searchedTags) {
                    auto strat = line.rfind(temp.first, 0);
                    if (strat == 0 && temp.second->empty()) {
                        *temp.second = line.substr(temp.first.size(), line.find("</", strat) - temp.first.size());
                    }
                }

                tryReadAllLanguageList(langList.back().layoutBaseISO639_2);
            }
        }
        if(line != "<variantList>") {
            return;
        }

        while (readAndTrimString(file, line) && line != "</variantList>") {
            readLangData(langList.back().variants);
        }
    };

    while (readAndTrimString(file, line)) {
        if(line != "<layoutList>") {
            continue;
        }

        while (readAndTrimString(file, line) && line != "</layoutList>") {
            readLangData(allCodes);
        }
    }

    file.close();

    return allCodes;
}

void
X11LayoutsParser::appendVectorUniq(const std::vector<String>& source, std::vector<String>& dst) {
    for(const auto& elem : source) {
        if(std::find_if(dst.begin(), dst.end(), [elem](const String& s) {return s == elem;}) == dst.end()) {
            dst.push_back(elem);
        }
    }
};

void
X11LayoutsParser::convertLayoutToISO639_2(std::vector<String> layoutNames,
                                          std::vector<String> layoutVariantNames,
                                          std::vector<String>& iso639_2Codes)
{
    auto allLang = getAllLanguageData();
    if(layoutNames.size() == layoutVariantNames.size()) {
        for (size_t i = 0; i < layoutNames.size(); i++) {
            auto langIter = std::find_if(allLang.begin(), allLang.end(), [n=layoutNames[i]](const Lang& l) {return l.name == n;});
            if(langIter == allLang.end()) {
                LOG((CLOG_WARN "Language \"%s\" is unknown", layoutNames[i].c_str()));
                continue;
            }

            std::vector<String>* toCopy = nullptr;
            if(layoutVariantNames[i].empty()) {
                toCopy = &langIter->layoutBaseISO639_2;
            }
            else {
                auto langVariantIter = std::find_if(langIter->variants.begin(), langIter->variants.end(),
                                                    [n=layoutVariantNames[i]](const Lang& l) {return l.name == n;});
                if(langIter == allLang.end()) {
                    LOG((CLOG_WARN "Variant \"%s\" of language \"%s\" is unknown", layoutVariantNames[i].c_str(), layoutNames[i].c_str()));
                    continue;
                }

                if(langVariantIter->layoutBaseISO639_2.empty()) {
                    toCopy = &langIter->layoutBaseISO639_2;
                }
                else {
                    toCopy = &langVariantIter->layoutBaseISO639_2;
                }
            }

            if(!toCopy) {
                LOG((CLOG_WARN "Logical error in X11 parser"));
                continue;
            }

            if(toCopy->empty()) {
                LOG((CLOG_WARN "Missed ISO 639-2 code for language \"%s\"", layoutNames[i].c_str()));
                continue;
            }

            appendVectorUniq(*toCopy, iso639_2Codes);
        }
    }
}

std::vector<String>
X11LayoutsParser::getX11LanguageList()
{
    std::vector<String> result;
    std::ifstream file("/etc/default/keyboard");
    if (!file.is_open()) {
        LOG((CLOG_WARN "Default x11 keyboard layouts file is missed."));
        return result;
    }

    const String layoutLinePrefix = "XKBLAYOUT=";
    const String variantLinePrefix = "XKBVARIANT=";

    auto splitLine = [](std::vector<String>& result, const String& line, char delim) {
        std::stringstream ss(line);
        String code;
        while(ss.good()) {
            getline(ss, code, delim);
            result.push_back(code);
        }
    };

    String line;
    std::vector<String> layoutNames;
    std::vector<String> layoutVariantNames;
    while (readAndTrimString(file, line)) {
        if (line.rfind(layoutLinePrefix, 0) == 0) {
            splitLine(layoutNames, line.substr(layoutLinePrefix.size()), ',');
        }
        else if (line.rfind(variantLinePrefix, 0) == 0) {
            splitLine(layoutVariantNames, line.substr(variantLinePrefix.size()), ',');
        }
    }

    std::vector<String> iso639_2Codes;
    convertLayoutToISO639_2(layoutNames, layoutVariantNames, iso639_2Codes);

    for (size_t i = 0; i < iso639_2Codes.size(); i++) {
        auto tableIter = std::find_if(ISO_Table.begin(), ISO_Table.end(),
                                            [n=iso639_2Codes[i]](const std::pair<String, String>& c) {return c.first == n;});
        if(tableIter == ISO_Table.end()) {
            LOG((CLOG_WARN "ISO 639-2 code \"%s\" is missed in table", iso639_2Codes[i].c_str()));
            continue;
        }

        appendVectorUniq({tableIter->second}, result);
    }

    return result;
}
