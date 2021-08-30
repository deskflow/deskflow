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

#pragma once
#include "base/String.h"

namespace pugi
{
    class xml_node;
}

class X11LayoutsParser {
public:
    static std::vector<String> getX11LanguageList();

private:
    struct Lang {
        String              name = "";
        String              shortDescr = "";
        String              descr = "";
        std::vector<String> layoutBaseISO639_2;
        std::vector<Lang>   variants;
    };

    static bool              readXMLConfigItemElem(const pugi::xml_node* root,
                                                   std::vector<Lang>& langList);

    static std::vector<Lang> getAllLanguageData();

    static void              appendVectorUniq(const std::vector<String>& source,
                                                    std::vector<String>& dst);

    static void              convertLayoutToISO639_2(std::vector<String> layoutNames,
                                                     std::vector<String> layoutVariantNames,
                                                     std::vector<String>& iso639_2Codes);
};
