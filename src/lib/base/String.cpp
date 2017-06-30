/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

#include "arch/Arch.h"
#include "base/String.h"
#include "common/common.h"
#include "common/stdvector.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <stdio.h>
#include <cstdarg>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cerrno>

namespace synergy {
namespace string {

String
format (const char* fmt, ...) {
    va_list args;
    va_start (args, fmt);
    String result = vformat (fmt, args);
    va_end (args);
    return result;
}

String
vformat (const char* fmt, va_list args) {
    // find highest indexed substitution and the locations of substitutions
    std::vector<size_t> pos;
    std::vector<size_t> width;
    std::vector<size_t> index;
    size_t maxIndex = 0;
    for (const char* scan = fmt; *scan != '\0'; ++scan) {
        if (*scan == '%') {
            ++scan;
            if (*scan == '\0') {
                break;
            } else if (*scan == '%') {
                // literal
                index.push_back (0);
                pos.push_back (static_cast<size_t> ((scan - 1) - fmt));
                width.push_back (2);
            } else if (*scan == '{') {
                // get argument index
                char* end;
                errno  = 0;
                long i = strtol (scan + 1, &end, 10);
                if (errno || (i < 0) || (*end != '}')) {
                    // invalid index -- ignore
                    scan = end - 1; // BUG if there are digits?
                } else {
                    index.push_back (i);
                    pos.push_back (static_cast<size_t> ((scan - 1) - fmt));
                    width.push_back (static_cast<size_t> ((end - scan) + 2));
                    if (i > maxIndex) {
                        maxIndex = i;
                    }
                    scan = end;
                }
            } else {
                // improper escape -- ignore
            }
        }
    }

    // get args
    std::vector<const char*> value;
    std::vector<size_t> length;
    value.push_back ("%");
    length.push_back (1);
    for (int i = 0; i < maxIndex; ++i) {
        const char* arg = va_arg (args, const char*);
        size_t len      = strlen (arg);
        value.push_back (arg);
        length.push_back (len);
    }

    // compute final length
    size_t resultLength = strlen (fmt);
    const int n         = static_cast<int> (pos.size ());
    for (int i = 0; i < n; ++i) {
        resultLength -= width[i];
        resultLength += length[index[i]];
    }

    // substitute
    String result;
    result.reserve (resultLength);
    size_t src = 0;
    for (int i = 0; i < n; ++i) {
        result.append (fmt + src, pos[i] - src);
        result.append (value[index[i]]);
        src = pos[i] + width[i];
    }
    result.append (fmt + src);

    return result;
}

String
sprintf (const char* fmt, ...) {
    char tmp[1024];
    char* buffer = tmp;
    int len      = (int) (sizeof (tmp) / sizeof (tmp[0]));
    String result;
    while (buffer != NULL) {
        // try printing into the buffer
        va_list args;
        va_start (args, fmt);
        int n = ARCH->vsnprintf (buffer, len, fmt, args);
        va_end (args);

        // if the buffer wasn't big enough then make it bigger and try again
        if (n < 0 || n > len) {
            if (buffer != tmp) {
                delete[] buffer;
            }
            len *= 2;
            buffer = new char[len];
        }

        // if it was big enough then save the string and don't try again
        else {
            result = buffer;
            if (buffer != tmp) {
                delete[] buffer;
            }
            buffer = NULL;
        }
    }

    return result;
}

void
findReplaceAll (String& subject, const String& find, const String& replace) {
    size_t pos = 0;
    while ((pos = subject.find (find, pos)) != String::npos) {
        subject.replace (pos, find.length (), replace);
        pos += replace.length ();
    }
}

String
removeFileExt (String filename) {
    size_t dot = filename.find_last_of ('.');

    if (dot == String::npos) {
        return filename;
    }

    return filename.substr (0, dot);
}

void
toHex (String& subject, int width, const char fill) {
    std::stringstream ss;
    ss << std::hex;
    for (unsigned int i = 0; i < subject.length (); i++) {
        ss << std::setw (width) << std::setfill (fill)
           << (int) (unsigned char) subject[i];
    }

    subject = ss.str ();
}

void
uppercase (String& subject) {
    std::transform (
        subject.begin (), subject.end (), subject.begin (), ::toupper);
}

void
removeChar (String& subject, const char c) {
    subject.erase (std::remove (subject.begin (), subject.end (), c),
                   subject.end ());
}

String
sizeTypeToString (size_t n) {
    std::stringstream ss;
    ss << n;
    return ss.str ();
}

size_t
stringToSizeType (String string) {
    std::istringstream iss (string);
    size_t value;
    iss >> value;
    return value;
}

std::vector<String>
splitString (String string, const char c) {
    std::vector<String> results;

    size_t head      = 0;
    size_t separator = string.find (c);
    while (separator != String::npos) {
        if (head != separator) {
            results.push_back (string.substr (head, separator - head));
        }
        head      = separator + 1;
        separator = string.find (c, head);
    }

    if (head < string.size ()) {
        results.push_back (string.substr (head, string.size () - head));
    }

    return results;
}

//
// CaselessCmp
//

bool
CaselessCmp::cmpEqual (const String::value_type& a,
                       const String::value_type& b) {
    // should use std::tolower but not in all versions of libstdc++ have it
    return tolower (a) == tolower (b);
}

bool
CaselessCmp::cmpLess (const String::value_type& a,
                      const String::value_type& b) {
    // should use std::tolower but not in all versions of libstdc++ have it
    return tolower (a) < tolower (b);
}

bool
CaselessCmp::less (const String& a, const String& b) {
    return std::lexicographical_compare (
        a.begin (),
        a.end (),
        b.begin (),
        b.end (),
        &synergy::string::CaselessCmp::cmpLess);
}

bool
CaselessCmp::equal (const String& a, const String& b) {
    return !(less (a, b) || less (b, a));
}

bool
CaselessCmp::operator() (const String& a, const String& b) const {
    return less (a, b);
}
}
}
