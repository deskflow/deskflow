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
#pragma once
#include "net/NetworkAddress.h"

class AutoArchSocket
{
public:
    explicit AutoArchSocket(IArchNetwork::EAddressFamily family);
    ~AutoArchSocket();

    AutoArchSocket(const AutoArchSocket&) = delete;
    AutoArchSocket& operator =(const AutoArchSocket&) = delete;

    void setNoDelayOnSocket(bool value = true);
    void setReuseAddrOnSocket(bool value = true);

    void listenOnSocket();
    ArchSocket acceptSocket();
    void bindSocket(const NetworkAddress& addr);
    bool connectSocket(const NetworkAddress& addr);
    void bindAndListen(const NetworkAddress& addr);

    void closeSocket();
    void closeSocketForRead();
    void closeSocketForWrite();

    size_t readSocket(UInt8* buffer, size_t size);
    size_t writeSocket(const UInt8* buffer, size_t size);
    void throwErrorOnSocket();

    bool isValid() const;
    ArchSocket getRawSocket() const;
    void operator =(ArchSocket socket);

private:
    ArchSocket m_socket = nullptr;
};
