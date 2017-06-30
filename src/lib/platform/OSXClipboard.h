/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "synergy/IClipboard.h"

#include <Carbon/Carbon.h>
#include <vector>

class IOSXClipboardConverter;

//! OS X clipboard implementation
class OSXClipboard : public IClipboard {
public:
    OSXClipboard ();
    virtual ~OSXClipboard ();

    //! Test if clipboard is owned by synergy
    static bool isOwnedBySynergy ();

    // IClipboard overrides
    virtual bool empty ();
    virtual void add (EFormat, const String& data);
    virtual bool open (Time) const;
    virtual void close () const;
    virtual Time getTime () const;
    virtual bool has (EFormat) const;
    virtual String get (EFormat) const;

    bool synchronize ();

private:
    void clearConverters ();

private:
    typedef std::vector<IOSXClipboardConverter*> ConverterList;

    mutable Time m_time;
    ConverterList m_converters;
    PasteboardRef m_pboard;
};

//! Clipboard format converter interface
/*!
This interface defines the methods common to all Scrap book format
*/
class IOSXClipboardConverter : public IInterface {
public:
    //! @name accessors
    //@{

    //! Get clipboard format
    /*!
    Return the clipboard format this object converts from/to.
    */
    virtual IClipboard::EFormat getFormat () const = 0;

    //! returns the scrap flavor type that this object converts from/to
    virtual CFStringRef getOSXFormat () const = 0;

    //! Convert from IClipboard format
    /*!
    Convert from the IClipboard format to the Carbon scrap format.
    The input data must be in the IClipboard format returned by
    getFormat().  The return data will be in the scrap
    format returned by getOSXFormat().
    */
    virtual String fromIClipboard (const String&) const = 0;

    //! Convert to IClipboard format
    /*!
    Convert from the carbon scrap format to the IClipboard format
    (i.e., the reverse of fromIClipboard()).
    */
    virtual String toIClipboard (const String&) const = 0;

    //@}
};
