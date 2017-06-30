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

#include "platform/XWindowsClipboard.h"

//! Convert to/from UTF-8 encoding
class XWindowsClipboardUTF8Converter : public IXWindowsClipboardConverter {
public:
    /*!
    \c name is converted to an atom and that is reported by getAtom().
    */
    XWindowsClipboardUTF8Converter (Display* display, const char* name);
    virtual ~XWindowsClipboardUTF8Converter ();

    // IXWindowsClipboardConverter overrides
    virtual IClipboard::EFormat getFormat () const;
    virtual Atom getAtom () const;
    virtual int getDataSize () const;
    virtual String fromIClipboard (const String&) const;
    virtual String toIClipboard (const String&) const;

private:
    Atom m_atom;
};
