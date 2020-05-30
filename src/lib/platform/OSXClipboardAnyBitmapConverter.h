/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
 * Patch by Ryan Chapman
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

#include "platform/OSXClipboard.h"

//! Convert to/from some text encoding
class OSXClipboardAnyBitmapConverter : public IOSXClipboardConverter {
public:
    OSXClipboardAnyBitmapConverter();
    virtual ~OSXClipboardAnyBitmapConverter();

    // IOSXClipboardConverter overrides
    virtual IClipboard::EFormat
                        getFormat() const;
    virtual CFStringRef    getOSXFormat() const = 0;
    virtual std::string fromIClipboard(const std::string &) const;
    virtual std::string toIClipboard(const std::string &) const;

protected:
    //! Convert from IClipboard format
    /*!
     Do UTF-8 conversion and linefeed conversion.
    */
    virtual std::string doFromIClipboard(const std::string&) const = 0;

    //! Convert to IClipboard format
    /*!
     Do UTF-8 conversion and Linefeed conversion.
    */
    virtual std::string doToIClipboard(const std::string&) const = 0;
};
