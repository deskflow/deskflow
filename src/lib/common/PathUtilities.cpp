/*
* barrier -- mouse and keyboard sharing utility
* Copyright (C) 2018 Debauchee Open Source Group
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

/*

These functions cover the vast majority of cases for different paths across
windows and unixes. They are not, however, fullproof and probably don't cover
fringe cases very well. The library below might be used as an alternative if
these implementations prove to be insufficient. As the library's readme states
it is simply a temporary band-aid until std::filesystem is integrated (C++17
has it in std::experimental) and this class should also be treated as such.

https://github.com/wjakob/filesystem/

*/

#include "PathUtilities.h"

// keep the default platform delimiter as the first in the list
#ifdef _WIN32
static const char *Delimiters = "\\/";
#else
static const char *Delimiters = "/";
#endif

static const char DefaultDelimiter = Delimiters[0];

std::string PathUtilities::basename(const std::string& path)
{
    return path.substr(path.find_last_of(Delimiters) + 1);
}

std::string PathUtilities::concat(const std::string& left, const std::string& right)
{
    // although npos is usually (-1) we can't count on that so handle it explicitly
    auto leftEnd = left.find_last_not_of(Delimiters);
    if (leftEnd == std::string::npos)
        leftEnd = 0;
    else
        ++leftEnd;
    auto rightStart = right.find_first_not_of(Delimiters, 0);
    if (rightStart == std::string::npos) {
        // both left/right are empty
        if (left.size() == 0 && right.size() == 0)
            return "";
        // right is full of delims, left is okay
        if (leftEnd > 0)
            return left.substr(0, leftEnd);
        // both left/right useless but at least one has delims
        return std::string(1, DefaultDelimiter);
    }
    if (leftEnd == 0) {
        // right is okay and not prefixed with delims, left is empty
        if (left.size() == 0 && rightStart == 0)
            return right.substr(rightStart);
        // (right is okay and prefixed with delims) OR left is full of delims
        return DefaultDelimiter + right.substr(rightStart);
    }
    // concatenation using both left and right
    return left.substr(0, leftEnd) + DefaultDelimiter + right.substr(rightStart);
}
