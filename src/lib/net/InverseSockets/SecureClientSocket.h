/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2022 Symless Ltd.
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
#include "SslApi.h"
#include "InverseClientSocket.h"

//! Secure socket
/*!
A secure socket using SSL.
*/
class SecureClientSocket : public InverseClientSocket {
public:
    SecureClientSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer, IArchNetwork::EAddressFamily family);
    SecureClientSocket(SecureClientSocket const &) =delete;
    SecureClientSocket(SecureClientSocket &&) =delete;

    SecureClientSocket& operator=(SecureClientSocket const &) =delete;
    SecureClientSocket& operator=(SecureClientSocket &&) =delete;

    // IDataSocket overrides
    void        connect(const NetworkAddress&) override;

    ISocketMultiplexerJob* newJob();
    bool                isFatal() const { return m_fatal; }
    void                setFatal(int code);
    int                 getRetry(int errorCode, int retry) const;
    bool                isSecureReady() const;
    void                secureConnect();
    void                secureAccept();
    int                 secureRead(void* buffer, int size, int& read);
    int                 secureWrite(const void* buffer, int size, int& wrote);
    EJobResult          doRead() override;
    EJobResult          doWrite() override;
    bool                loadCertificates(const std::string& CertFile);

private:
    // SSL
    void                initContext(bool server);
    int                 secureAccept(int s);
    int                 secureConnect(int s);
    void                checkResult(int n, int& retry);
    void                disconnect();

    ISocketMultiplexerJob*
                        serviceConnect(ISocketMultiplexerJob*,
                            bool, bool, bool);

    ISocketMultiplexerJob*
                        serviceAccept(ISocketMultiplexerJob*,
                            bool, bool, bool);

    void                handleTCPConnected(const Event&, void*);

    synergy::ssl::SslApi  m_ssl{false};
    bool                  m_secureReady = false;
    bool                  m_fatal = false;
};
