/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2016 Symless Ltd.
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

#include "net/TCPSocket.h"
#include "net/XSocket.h"

class IEventQueue;
class SocketMultiplexer;
class ISocketMultiplexerJob;

struct Ssl;

//! Secure socket
/*!
A secure socket using SSL.
*/
class SecureSocket : public TCPSocket {
public:
    SecureSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer, IArchNetwork::EAddressFamily family);
    SecureSocket(IEventQueue* events,
        SocketMultiplexer* socketMultiplexer,
        ArchSocket socket);
    ~SecureSocket();

    // ISocket overrides
    void                close();

    // IDataSocket overrides
    virtual void        connect(const NetworkAddress&);
    
    std::unique_ptr<ISocketMultiplexerJob> newJob() override;
    bool                isFatal() const { return m_fatal; }
    void                isFatal(bool b) { m_fatal = b; }
    bool                isSecureReady();
    void                secureConnect();
    void                secureAccept();
    int                    secureRead(void* buffer, int size, int& read);
    int                    secureWrite(const void* buffer, int size, int& wrote);
    EJobResult            doRead();
    EJobResult            doWrite();
    void                initSsl(bool server);
    bool                loadCertificates(String& CertFile);

private:
    // SSL
    void                initContext(bool server);
    void                createSSL();
    int                    secureAccept(int s);
    int                    secureConnect(int s);
    bool                showCertificate();
    void                checkResult(int n, int& retry);
    void                showError(const char* reason = NULL);
    String                getError();
    void                disconnect();
    void                formatFingerprint(String& fingerprint,
                                            bool hex = true,
                                            bool separator = true);
    bool                verifyCertFingerprint();

    MultiplexerJobStatus serviceConnect(ISocketMultiplexerJob*, bool, bool, bool);
    MultiplexerJobStatus serviceAccept(ISocketMultiplexerJob*, bool, bool, bool);

    void                showSecureConnectInfo();
    void                showSecureLibInfo();
    void                showSecureCipherInfo();
    
    void                handleTCPConnected(const Event& event, void*);

    void freeSSLResources();

private:
    Ssl*                m_ssl;
    bool                m_secureReady;
    bool                m_fatal;
};
