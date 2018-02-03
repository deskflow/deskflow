/*
* barrier -- mouse and keyboard sharing utility
* Copyright (C) 2018 Deuauche Open Source Group
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

#include "ImmuneKeysReader.h"

#include <fstream>

const std::size_t AllocatedLineSize = 1024;
const char CommentChar = '#';

static void add_key(const char * const buffer, std::vector<DWORD> &keys)
{
    const char *first;
    // skip spaces. ignore blank lines and comment lines
    for (first = buffer; *first == ' '; ++first);
    if (*first != 0 && *first != CommentChar)
        keys.emplace_back(std::stoul(first, 0, 0));
}

/*static*/ bool ImmuneKeysReader::get_list(const char * const path, std::vector<DWORD> &keys, std::string &badline)
{
    // default values for return params
    keys.clear();
    badline.clear();
    std::ifstream stream(path, std::ios::in);
    if (stream.is_open()) {
        // size includes the null-terminator
        char buffer[AllocatedLineSize];
        while (stream.getline(&buffer[0], AllocatedLineSize)) {
            try {
                add_key(buffer, keys);
            } catch (...) {
                badline = buffer;
                return false;
            }
        }
    }
    return true;
}