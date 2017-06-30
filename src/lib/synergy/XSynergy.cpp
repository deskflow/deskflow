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

#include "synergy/XSynergy.h"
#include "base/String.h"

//
// XBadClient
//

String
XBadClient::getWhat () const throw () {
    return "XBadClient";
}


//
// XIncompatibleClient
//

XIncompatibleClient::XIncompatibleClient (int major, int minor)
    : m_major (major), m_minor (minor) {
    // do nothing
}

int
XIncompatibleClient::getMajor () const throw () {
    return m_major;
}

int
XIncompatibleClient::getMinor () const throw () {
    return m_minor;
}

String
XIncompatibleClient::getWhat () const throw () {
    return format ("XIncompatibleClient",
                   "incompatible client %{1}.%{2}",
                   synergy::string::sprintf ("%d", m_major).c_str (),
                   synergy::string::sprintf ("%d", m_minor).c_str ());
}


//
// XDuplicateClient
//

XDuplicateClient::XDuplicateClient (const String& name) : m_name (name) {
    // do nothing
}

const String&
XDuplicateClient::getName () const throw () {
    return m_name;
}

String
XDuplicateClient::getWhat () const throw () {
    return format (
        "XDuplicateClient", "duplicate client %{1}", m_name.c_str ());
}


//
// XUnknownClient
//

XUnknownClient::XUnknownClient (const String& name) : m_name (name) {
    // do nothing
}

const String&
XUnknownClient::getName () const throw () {
    return m_name;
}

String
XUnknownClient::getWhat () const throw () {
    return format ("XUnknownClient", "unknown client %{1}", m_name.c_str ());
}


//
// XExitApp
//

XExitApp::XExitApp (int code) : m_code (code) {
    // do nothing
}

int
XExitApp::getCode () const throw () {
    return m_code;
}

String
XExitApp::getWhat () const throw () {
    return format ("XExitApp",
                   "exiting with code %{1}",
                   synergy::string::sprintf ("%d", m_code).c_str ());
}
