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

#include "common/stdpre.h"
#if HAVE_ISTREAM
#include <istream>
#else
#include <iostream>
#endif
#include "common/stdpost.h"

#if defined(_MSC_VER) && _MSC_VER <= 1200
// VC++6 istream has no overloads for __int* types, .NET does
inline std::istream&
operator>> (std::istream& s, SInt8& i) {
    return s >> (signed char&) i;
}
inline std::istream&
operator>> (std::istream& s, SInt16& i) {
    return s >> (short&) i;
}
inline std::istream&
operator>> (std::istream& s, SInt32& i) {
    return s >> (int&) i;
}
inline std::istream&
operator>> (std::istream& s, UInt8& i) {
    return s >> (unsigned char&) i;
}
inline std::istream&
operator>> (std::istream& s, UInt16& i) {
    return s >> (unsigned short&) i;
}
inline std::istream&
operator>> (std::istream& s, UInt32& i) {
    return s >> (unsigned int&) i;
}
#endif
