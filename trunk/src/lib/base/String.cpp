/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Bolton Software Ltd.
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "base/String.h"

#include <memory>
#include <cstdarg>

void
find_replace_all(
	CString& subject,
	const CString& find,
	const CString& replace)
{
    size_t pos = 0;
    while ((pos = subject.find(find, pos)) != CString::npos) {
         subject.replace(pos, find.length(), replace);
         pos += replace.length();
    }
}

CString
string_format(const CString format, ...)
{
	// reserve 2 times as much as the length of the format
    size_t final, n = format.size() * 2;

    CString str;
    std::unique_ptr<char[]> formatted;
    va_list ap;

    while (true) {
        
		// wrap the plain char array in unique_ptr
		formatted.reset(new char[n]);

        strcpy(&formatted[0], format.c_str());
        va_start(ap, format);
        final = vsnprintf(&formatted[0], n, format.c_str(), ap);
        va_end(ap);

        if (final < 0 || final >= n) {
            n += abs(static_cast<int>(final - n + 1));
		}
		else {
            break;
		}
    }

    return CString(formatted.get());
}
