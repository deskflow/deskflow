/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include "CXWindowsClipboardUCS2Converter.h"
#include "CUnicode.h"

//
// CXWindowsClipboardUCS2Converter
//

CXWindowsClipboardUCS2Converter::CXWindowsClipboardUCS2Converter(
				Display* display, const char* name) :
	m_atom(XInternAtom(display, name, False))
{
	// do nothing
}

CXWindowsClipboardUCS2Converter::~CXWindowsClipboardUCS2Converter()
{
	// do nothing
}

IClipboard::EFormat
CXWindowsClipboardUCS2Converter::getFormat() const
{
	return IClipboard::kText;
}

Atom
CXWindowsClipboardUCS2Converter::getAtom() const
{
	return m_atom;
}

int
CXWindowsClipboardUCS2Converter::getDataSize() const
{
	return 16;
}

CString
CXWindowsClipboardUCS2Converter::fromIClipboard(const CString& data) const
{
	return CUnicode::UTF8ToUCS2(data);
}

CString
CXWindowsClipboardUCS2Converter::toIClipboard(const CString& data) const
{
	return CUnicode::UCS2ToUTF8(data);
}
