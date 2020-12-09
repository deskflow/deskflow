/*
 * barrier -- mouse and keyboard sharing utility
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

#include "platform/MSWindowsClipboardAnyTextConverter.h"

//! Convert to/from HTML encoding
class MSWindowsClipboardHTMLConverter :
                public MSWindowsClipboardAnyTextConverter {
public:
    MSWindowsClipboardHTMLConverter();
    virtual ~MSWindowsClipboardHTMLConverter();

    // IMSWindowsClipboardConverter overrides
    virtual IClipboard::EFormat
                        getFormat() const;
    virtual UINT        getWin32Format() const;

protected:
    // MSWindowsClipboardAnyTextConverter overrides
    virtual std::string doFromIClipboard(const std::string&) const;
    virtual std::string doToIClipboard(const std::string&) const;

private:
    std::string findArg(const std::string& data, const std::string& name) const;

private:
    UINT                m_format;
};
