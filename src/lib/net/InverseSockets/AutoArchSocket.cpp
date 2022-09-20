/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2022 Symless Ltd.
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
#include "AutoArchSocket.h"

#include "base/Log.h"
#include "arch/Arch.h"
#include "arch/XArch.h"
#include "net/XSocket.h"

AutoArchSocket::AutoArchSocket(IArchNetwork::EAddressFamily family)
{
    try {
        m_socket = ARCH->newSocket(family, IArchNetwork::kSTREAM);
        LOG((CLOG_DEBUG "Opening new socket: %08X", m_socket));
    }
    catch (const XArchNetwork& e) {
        throw XSocketCreate(e.what());
    }
}

AutoArchSocket::~AutoArchSocket()
{
    closeSocket();
}

void AutoArchSocket::setNoDelayOnSocket(bool value)
{
    if (m_socket) {
        ARCH->setNoDelayOnSocket(m_socket, value);
    }
}

void AutoArchSocket::setReuseAddrOnSocket(bool value)
{
    if (m_socket) {
        ARCH->setReuseAddrOnSocket(m_socket, value);
    }
}

void AutoArchSocket::closeSocket()
{
    if (m_socket) {
        try {
            LOG((CLOG_DEBUG "Closing socket: %08X", m_socket));
            ARCH->closeSocket(m_socket);
            m_socket = nullptr;
        }
        catch (const XArchNetwork& e) {
            // ignore, there's not much we can do
            LOG((CLOG_WARN "error closing socket: %s", e.what()));
        }
    }
    else {
        LOG((CLOG_WARN "error closing socket because the socket already closed"));
    }
}

void AutoArchSocket::bindSocket(const NetworkAddress &addr)
{
    if (m_socket) {
        try {
            ARCH->bindSocket(m_socket, addr.getAddress());
        }
        catch (const XArchNetworkAddressInUse& e) {
            throw XSocketAddressInUse(e.what());
        }
        catch (const XArchNetwork& e) {
            throw XSocketBind(e.what());
        }
    }
}

void AutoArchSocket::bindAndListen(const NetworkAddress &addr)
{
    try {
        setReuseAddrOnSocket();
        bindSocket(addr);
        listenOnSocket();
    }
    catch (XArchNetworkAddressInUse& e) {
        throw XSocketAddressInUse(e.what());
    }
    catch (XArchNetwork& e) {
        throw XSocketBind(e.what());
    }
}

void AutoArchSocket::listenOnSocket()
{
    if (m_socket) {
        ARCH->listenOnSocket(m_socket);
    }
}

ArchSocket AutoArchSocket::acceptSocket()
{
    return ARCH->acceptSocket(m_socket, nullptr);
}

void AutoArchSocket::closeSocketForRead()
{
    if (m_socket) {
        try {
            ARCH->closeSocketForRead(m_socket);
        }
        catch (const XArchNetwork& e) {
            // ignore, there's not much we can do
            LOG((CLOG_WARN "error closing socket: %s", e.what()));
        }
    }
}

void AutoArchSocket::closeSocketForWrite()
{
    if (m_socket) {
        try {
            ARCH->closeSocketForWrite(m_socket);
        }
        catch (const XArchNetwork& e) {
            // ignore, there's not much we can do
            LOG((CLOG_WARN "error closing socket: %s", e.what()));
        }
    }
}

bool AutoArchSocket::connectSocket(const NetworkAddress &addr)
{
    bool result = false;

    if (m_socket) {
        // turn off Nagle algorithm. we send lots of very short messages
        // that should be sent without (much) delay. for example, the
        // mouse motion messages are much less useful if they're delayed.
        setNoDelayOnSocket();
        result = ARCH->connectSocket(m_socket, addr.getAddress());
    }

    return result;
}

size_t AutoArchSocket::readSocket(UInt8* buffer, size_t size)
{
    size_t result = 0;

    if (m_socket) {
        result = ARCH->readSocket(m_socket, buffer, size);
    }

    return result;
}

size_t AutoArchSocket::writeSocket(const UInt8* buffer, size_t size)
{
    size_t result = 0;

    if (m_socket) {
        result = ARCH->writeSocket(m_socket, buffer, size);
    }

    return result;
}

void AutoArchSocket::throwErrorOnSocket()
{
    if (m_socket) {
        ARCH->throwErrorOnSocket(m_socket);
    }
}

ArchSocket AutoArchSocket::getRawSocket() const
{
    return m_socket;
}
