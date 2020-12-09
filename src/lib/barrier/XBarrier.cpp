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

#include "barrier/XBarrier.h"
#include "base/String.h"

//
// XBadClient
//

String XBadClient::getWhat() const noexcept
{
    return "XBadClient";
}


//
// XIncompatibleClient
//

XIncompatibleClient::XIncompatibleClient(int major, int minor) :
    m_major(major),
    m_minor(minor)
{
    // do nothing
}

int XIncompatibleClient::getMajor() const noexcept
{
    return m_major;
}

int XIncompatibleClient::getMinor() const noexcept
{
    return m_minor;
}

String XIncompatibleClient::getWhat() const noexcept
{
    return format("XIncompatibleClient", "incompatible client %{1}.%{2}",
                                barrier::string::sprintf("%d", m_major).c_str(),
                                barrier::string::sprintf("%d", m_minor).c_str());
}


//
// XDuplicateClient
//

XDuplicateClient::XDuplicateClient(const String& name) :
    m_name(name)
{
    // do nothing
}

const String& XDuplicateClient::getName() const noexcept
{
    return m_name;
}

String XDuplicateClient::getWhat() const noexcept
{
    return format("XDuplicateClient", "duplicate client %{1}", m_name.c_str());
}


//
// XUnknownClient
//

XUnknownClient::XUnknownClient(const String& name) :
    m_name(name)
{
    // do nothing
}

const String& XUnknownClient::getName() const noexcept
{
    return m_name;
}

String XUnknownClient::getWhat() const noexcept
{
    return format("XUnknownClient", "unknown client %{1}", m_name.c_str());
}


//
// XExitApp
//

XExitApp::XExitApp(int code) :
    m_code(code)
{
    // do nothing
}

int XExitApp::getCode() const noexcept
{
    return m_code;
}

String XExitApp::getWhat() const noexcept
{
    return format(
        "XExitApp", "exiting with code %{1}",
        barrier::string::sprintf("%d", m_code).c_str());
}
