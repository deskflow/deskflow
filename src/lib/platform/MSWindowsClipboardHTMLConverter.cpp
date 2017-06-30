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

#include "platform/MSWindowsClipboardHTMLConverter.h"

#include "base/String.h"

//
// MSWindowsClipboardHTMLConverter
//

MSWindowsClipboardHTMLConverter::MSWindowsClipboardHTMLConverter () {
    m_format = RegisterClipboardFormat ("HTML Format");
}

MSWindowsClipboardHTMLConverter::~MSWindowsClipboardHTMLConverter () {
    // do nothing
}

IClipboard::EFormat
MSWindowsClipboardHTMLConverter::getFormat () const {
    return IClipboard::kHTML;
}

UINT
MSWindowsClipboardHTMLConverter::getWin32Format () const {
    return m_format;
}

String
MSWindowsClipboardHTMLConverter::doFromIClipboard (const String& data) const {
    // prepare to CF_HTML format prefix and suffix
    String prefix ("Version:0.9\r\nStartHTML:0000000105\r\n"
                   "EndHTML:ZZZZZZZZZZ\r\n"
                   "StartFragment:XXXXXXXXXX\r\nEndFragment:YYYYYYYYYY\r\n"
                   "<!DOCTYPE><HTML><BODY><!--StartFragment-->");
    String suffix ("<!--EndFragment--></BODY></HTML>\r\n");

    // Get byte offsets for header
    UInt32 StartFragment = (UInt32) prefix.size ();
    UInt32 EndFragment   = StartFragment + (UInt32) data.size ();
    // StartHTML is constant by the design of the prefix
    UInt32 EndHTML = EndFragment + (UInt32) suffix.size ();

    prefix.replace (prefix.find ("XXXXXXXXXX"),
                    10,
                    synergy::string::sprintf ("%010u", StartFragment));
    prefix.replace (prefix.find ("YYYYYYYYYY"),
                    10,
                    synergy::string::sprintf ("%010u", EndFragment));
    prefix.replace (prefix.find ("ZZZZZZZZZZ"),
                    10,
                    synergy::string::sprintf ("%010u", EndHTML));

    // concatenate
    prefix += data;
    prefix += suffix;
    return prefix;
}

String
MSWindowsClipboardHTMLConverter::doToIClipboard (const String& data) const {
    // get fragment start/end args
    String startArg = findArg (data, "StartFragment");
    String endArg   = findArg (data, "EndFragment");
    if (startArg.empty () || endArg.empty ()) {
        return String ();
    }

    // convert args to integers
    SInt32 start = (SInt32) atoi (startArg.c_str ());
    SInt32 end   = (SInt32) atoi (endArg.c_str ());
    if (start <= 0 || end <= 0 || start >= end) {
        return String ();
    }

    // extract the fragment
    return data.substr (start, end - start);
}

String
MSWindowsClipboardHTMLConverter::findArg (const String& data,
                                          const String& name) const {
    String::size_type i = data.find (name);
    if (i == String::npos) {
        return String ();
    }
    i = data.find_first_of (":\r\n", i);
    if (i == String::npos || data[i] != ':') {
        return String ();
    }
    i = data.find_first_of ("0123456789\r\n", i + 1);
    if (i == String::npos || data[i] == '\r' || data[i] == '\n') {
        return String ();
    }
    String::size_type j = data.find_first_not_of ("0123456789", i);
    if (j == String::npos) {
        j = data.size ();
    }
    return data.substr (i, j - i);
}
