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

#include "base/Log.h"
#include "synergy/X11LayoutsParser.h"
#include "ISO639Table.h"
#include "pugixml.hpp"

bool
X11LayoutsParser::readXMLConfigItemElem(const pugi::xml_node* root, std::vector<Lang>& langList)
{
    auto configItemElem = root->child("configItem");
    if(!configItemElem) {
        LOG((CLOG_WARN "Failed to read \"configItem\" in xml file"));
        return false;
    }

    langList.emplace_back(Lang());
    auto nameElem = configItemElem.child("name");
    if(nameElem) {
        langList.back().name = nameElem.text().as_string();
    }

    auto languageListElem = configItemElem.child("languageList");
    if(languageListElem) {
        for (pugi::xml_node isoElem : languageListElem.children("iso639Id")) {
            langList.back().layoutBaseISO639_2.emplace_back(isoElem.text().as_string());
        }
    }

    return true;
}

std::vector<X11LayoutsParser::Lang>
X11LayoutsParser::getAllLanguageData(const String& pathToEvdevFile)
{
    std::vector<Lang> allCodes;
    pugi::xml_document doc;
    if(!doc.load_file(pathToEvdevFile.c_str())) {
        LOG((CLOG_WARN "Failed to open %s", pathToEvdevFile.c_str()));
        return allCodes;
    }

    auto xkbConfigElem = doc.child("xkbConfigRegistry");
    if(!xkbConfigElem) {
        LOG((CLOG_WARN "Failed to read xkbConfigRegistry in %s", pathToEvdevFile.c_str()));
        return allCodes;
    }

    auto layoutListElem = xkbConfigElem.child("layoutList");
    if(!layoutListElem) {
        LOG((CLOG_WARN "Failed to read layoutList in %s", pathToEvdevFile.c_str()));
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

void
X11LayoutsParser::appendVectorUniq(const std::vector<String>& source, std::vector<String>& dst) {
    for(const auto& elem : source) {
        if(std::find_if(dst.begin(), dst.end(), [elem](const String& s) {return s == elem;}) == dst.end()) {
            dst.push_back(elem);
        }
    }
};

void
X11LayoutsParser::convertLayoutToISO639_2(const String& pathToEvdevFile,
                                          std::vector<String> layoutNames,
                                          std::vector<String> layoutVariantNames,
                                          std::vector<String>& iso639_2Codes)
{
    if(layoutNames.size() != layoutVariantNames.size()) {
        LOG((CLOG_WARN "Error in language layout or language layout variants list"));
        return;
    }

    auto allLang = getAllLanguageData(pathToEvdevFile);
    for (size_t i = 0; i < layoutNames.size(); i++) {
        auto langIter = std::find_if(allLang.begin(), allLang.end(), [n=layoutNames[i]](const Lang& l) {return l.name == n;});
        if(langIter == allLang.end()) {
            LOG((CLOG_WARN "Language \"%s\" is unknown", layoutNames[i].c_str()));
            continue;
        }

        const std::vector<String>* toCopy = nullptr;
        if(layoutVariantNames[i].empty()) {
            toCopy = &langIter->layoutBaseISO639_2;
        }
        else {
            auto langVariantIter = std::find_if(langIter->variants.begin(), langIter->variants.end(),
                                                [n=layoutVariantNames[i]](const Lang& l) {return l.name == n;});
            if(langVariantIter == langIter->variants.end()) {
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

std::vector<String>
X11LayoutsParser::getX11LanguageList(const String& pathToKeyboardFile, const String& pathToEvdevFile)
{
    std::vector<String> result;
    std::ifstream file(pathToKeyboardFile);
    if (!file.is_open()) {
        LOG((CLOG_WARN "x11 keyboard layouts file \"%s\" is missed.", pathToKeyboardFile.c_str()));
        return result;
    }

    const String layoutLinePrefix = "XKBLAYOUT=";
    const String variantLinePrefix = "XKBVARIANT=";

    auto splitLine = [](std::vector<String>& splitted, const String& line, char delim) {
        std::stringstream ss(line);
        String code;
        while(ss.good()) {
            getline(ss, code, delim);
            splitted.push_back(code);
        }
    };

    String line;
    std::vector<String> layoutNames;
    std::vector<String> layoutVariantNames;
    while (std::getline(file, line)) {
        if (line.rfind(layoutLinePrefix, 0) == 0) {
            splitLine(layoutNames, line.substr(layoutLinePrefix.size()), ',');
        }
        else if (line.rfind(variantLinePrefix, 0) == 0) {
            splitLine(layoutVariantNames, line.substr(variantLinePrefix.size()), ',');
        }
    }

    std::vector<String> iso639_2Codes;
    convertLayoutToISO639_2(pathToEvdevFile, layoutNames, layoutVariantNames, iso639_2Codes);

    for (const auto& isoCode : iso639_2Codes) {
        auto tableIter = std::find_if(ISO_Table.begin(), ISO_Table.end(),
                                            [&isoCode](const std::pair<String, String>& c) {return c.first == isoCode;});
        if(tableIter == ISO_Table.end()) {
            LOG((CLOG_WARN "ISO 639-2 code \"%s\" is missed in table", isoCode.c_str()));
            continue;
        }

        appendVectorUniq({tableIter->second}, result);
    }

    return result;
}
