/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "platform/XWindowsClipboardUCS2Converter.h"

#include "base/Unicode.h"

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

String
CXWindowsClipboardUCS2Converter::fromIClipboard(const String& data) const
{
	return Unicode::UTF8ToUCS2(data);
}

String
CXWindowsClipboardUCS2Converter::toIClipboard(const String& data) const
{
	return Unicode::UCS2ToUTF8(data);
}
