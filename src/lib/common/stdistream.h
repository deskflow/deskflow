/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
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
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
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
inline
std::istream& operator>>(std::istream& s, SInt8& i)
{ return s >> (signed char&)i; }
inline
std::istream& operator>>(std::istream& s, SInt16& i)
{ return s >> (short&)i; }
inline
std::istream& operator>>(std::istream& s, SInt32& i)
{ return s >> (int&)i; }
inline
std::istream& operator>>(std::istream& s, UInt8& i)
{ return s >> (unsigned char&)i; }
inline
std::istream& operator>>(std::istream& s, UInt16& i)
{ return s >> (unsigned short&)i; }
inline
std::istream& operator>>(std::istream& s, UInt32& i)
{ return s >> (unsigned int&)i; }
#endif
