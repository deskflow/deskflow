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

#include "net/NetworkAddress.h"

#include "net/XSocket.h"
#include "arch/Arch.h"
#include "arch/XArch.h"

#include <cstdlib>

//
// NetworkAddress
//

static bool parse_address(const std::string& address, std::string& host, int& port)
{
    /* Three cases ---
    * brackets:  parse inside for host, check end for port as :INTEGER. DONE
    * one colon: ipv4 address with port. DONE
    * otherwise: all host, no port. DONE
    *
    * very, very little error checking. depends on address being trimmed before call.
    *
    * does not override port with a default value if no port was found in address.
    */

    if (address[0] == '[') {
        // bracketed host possibly followed by port as :INTEGER
        auto endBracket = address.find(']', 1);
        if (endBracket == std::string::npos)
            return false;
        host = address.substr(1, endBracket - 1);
        if (endBracket + 1 < address.length()) {
            // port follows (or garbage)
            if (address[endBracket + 1] != ':')
                return false;
            port = std::strtol(&address[endBracket + 2], nullptr, 10);
        }
    } else {
        auto colon = address.find(':');
        if (colon != std::string::npos && address.find(':', colon + 1) == std::string::npos) {
            // one single colon, must be ipv4 with port
            host = address.substr(0, colon);
            port = std::strtol(&address[colon + 1], nullptr, 10);
        } else {
            // no colons (ipv4) or more than one colon (ipv6), both without port
            host = address;
        }
    }

    return true;
}

// name re-resolution adapted from a patch by Brent Priddy.

NetworkAddress::NetworkAddress() :
    m_address(NULL),
    m_hostname(),
    m_port(0)
{
    // note -- make no calls to Network socket interface here;
    // we're often called prior to Network::init().
}

NetworkAddress::NetworkAddress(int port) :
    m_address(NULL),
    m_hostname(),
    m_port(port)
{
    checkPort();
    m_address = ARCH->newAnyAddr(IArchNetwork::kINET);
    ARCH->setAddrPort(m_address, m_port);
}

NetworkAddress::NetworkAddress(const NetworkAddress& addr) :
    m_address(addr.m_address != NULL ? ARCH->copyAddr(addr.m_address) : NULL),
    m_hostname(addr.m_hostname),
    m_port(addr.m_port)
{
    // do nothing
}

NetworkAddress::NetworkAddress(const String& hostname, int port) :
    m_address(NULL),
    m_hostname(hostname),
    m_port(port)
{
    if (!parse_address(hostname, m_hostname, m_port))
        throw XSocketAddress(XSocketAddress::kUnknown,
            m_hostname, m_port);
    checkPort();
}

NetworkAddress::~NetworkAddress()
{
    if (m_address != NULL) {
        ARCH->closeAddr(m_address);
    }
}

NetworkAddress&
NetworkAddress::operator=(const NetworkAddress& addr)
{
    ArchNetAddress newAddr = NULL;
    if (addr.m_address != NULL) {
        newAddr = ARCH->copyAddr(addr.m_address);
    }
    if (m_address != NULL) {
        ARCH->closeAddr(m_address);
    }
    m_address  = newAddr;
    m_hostname = addr.m_hostname;
    m_port     = addr.m_port;
    return *this;
}

void
NetworkAddress::resolve()
{
    // discard previous address
    if (m_address != NULL) {
        ARCH->closeAddr(m_address);
        m_address = NULL;
    }

    try {
        // if hostname is empty then use wildcard address otherwise look
        // up the name.
        if (m_hostname.empty()) {
            m_address = ARCH->newAnyAddr(IArchNetwork::kINET6);
        }
        else {
            m_address = ARCH->nameToAddr(m_hostname);
        }
    }
    catch (XArchNetworkNameUnknown&) {
        throw XSocketAddress(XSocketAddress::kNotFound, m_hostname, m_port);
    }
    catch (XArchNetworkNameNoAddress&) {
        throw XSocketAddress(XSocketAddress::kNoAddress, m_hostname, m_port);
    }
    catch (XArchNetworkNameUnsupported&) {
        throw XSocketAddress(XSocketAddress::kUnsupported, m_hostname, m_port);
    }
    catch (XArchNetworkName&) {
        throw XSocketAddress(XSocketAddress::kUnknown, m_hostname, m_port);
    }

    // set port in address
    ARCH->setAddrPort(m_address, m_port);
}

bool
NetworkAddress::operator==(const NetworkAddress& addr) const
{
    return ARCH->isEqualAddr(m_address, addr.m_address);
}

bool
NetworkAddress::operator!=(const NetworkAddress& addr) const
{
    return !operator==(addr);
}

bool
NetworkAddress::isValid() const
{
    return (m_address != NULL);
}

const ArchNetAddress&
NetworkAddress::getAddress() const
{
    return m_address;
}

int
NetworkAddress::getPort() const
{
    return m_port;
}

String
NetworkAddress::getHostname() const
{
    return m_hostname;
}

void
NetworkAddress::checkPort()
{
    // check port number
    if (m_port <= 0 || m_port > 65535) {
        throw XSocketAddress(XSocketAddress::kBadPort, m_hostname, m_port);
    }
}
