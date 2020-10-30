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

#include "net/XSocket.h"
#include "base/String.h"

//
// XSocketAddress
//

XSocketAddress::XSocketAddress(EError error, const std::string& hostname, int port) noexcept :
    m_error(error),
    m_hostname(hostname),
    m_port(port)
{
    // do nothing
}

XSocketAddress::EError XSocketAddress::getError() const noexcept
{
    return m_error;
}

std::string XSocketAddress::getHostname() const noexcept
{
    return m_hostname;
}

int XSocketAddress::getPort() const noexcept
{
    return m_port;
}

std::string XSocketAddress::getWhat() const noexcept
{
    static const char* s_errorID[] = {
        "XSocketAddressUnknown",
        "XSocketAddressNotFound",
        "XSocketAddressNoAddress",
        "XSocketAddressUnsupported",
        "XSocketAddressBadPort"
    };
    static const char* s_errorMsg[] = {
        "unknown error for: %{1}:%{2}",
        "address not found for: %{1}",
        "no address for: %{1}",
        "unsupported address for: %{1}",
        "invalid port"                // m_port may not be set to the bad port
    };
    return format(s_errorID[m_error], s_errorMsg[m_error],
                                m_hostname.c_str(),
                                barrier::string::sprintf("%d", m_port).c_str());
}


//
// XSocketIOClose
//

std::string XSocketIOClose::getWhat() const noexcept
{
    return format("XSocketIOClose", "close: %{1}", what());
}


//
// XSocketBind
//

std::string XSocketBind::getWhat() const noexcept
{
    return format("XSocketBind", "cannot bind address: %{1}", what());
}


//
// XSocketConnect
//

std::string XSocketConnect::getWhat() const noexcept
{
    return format("XSocketConnect", "cannot connect socket: %{1}", what());
}


//
// XSocketCreate
//

std::string XSocketCreate::getWhat() const noexcept
{
    return format("XSocketCreate", "cannot create socket: %{1}", what());
}
