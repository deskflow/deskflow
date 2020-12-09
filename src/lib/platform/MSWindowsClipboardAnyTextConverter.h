/*
 * barrier -- mouse and keyboard sharing utility
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

#include "platform/MSWindowsClipboard.h"

//! Convert to/from some text encoding
class MSWindowsClipboardAnyTextConverter :
                public IMSWindowsClipboardConverter {
public:
    MSWindowsClipboardAnyTextConverter();
    virtual ~MSWindowsClipboardAnyTextConverter();

    // IMSWindowsClipboardConverter overrides
    virtual IClipboard::EFormat
                        getFormat() const;
    virtual UINT        getWin32Format() const = 0;
    virtual HANDLE fromIClipboard(const std::string&) const;
    virtual std::string toIClipboard(HANDLE) const;

protected:
    //! Convert from IClipboard format
    /*!
    Do UTF-8 conversion only.  Memory handle allocation and
    linefeed conversion is done by this class.  doFromIClipboard()
    must include the nul terminator in the returned string (not
    including the std::string's nul terminator).
    */
    virtual std::string doFromIClipboard(const std::string&) const = 0;

    //! Convert to IClipboard format
    /*!
    Do UTF-8 conversion only.  Memory handle allocation and
    linefeed conversion is done by this class.
    */
    virtual std::string doToIClipboard(const std::string&) const = 0;

private:
    std::string convertLinefeedToWin32(const std::string&) const;
    std::string convertLinefeedToUnix(const std::string&) const;
};
