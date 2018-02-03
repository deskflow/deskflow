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

#pragma once

#include <vector>
#include <string>

// let's not import all of Windows just to get this typedef
typedef unsigned long DWORD;

class ImmuneKeysReader
{
public:
    static bool get_list(const char * const path, std::vector<DWORD> &keys, std::string &badLine);

private:
    // static class
    explicit ImmuneKeysReader() {}
};
