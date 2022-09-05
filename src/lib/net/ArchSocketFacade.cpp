#include "ArchSocketFacade.h"

#include "net/XSocket.h"
#include "arch/XArch.h"
#include "base/Log.h"

ArchSocketFacade::ArchSocketFacade(IArchNetwork::EAddressFamily family)
{
    try {
        m_socket = ARCH->newSocket(family, IArchNetwork::kSTREAM);
        LOG((CLOG_DEBUG "Opening new socket: %08X", m_socket));
    }
    catch (const XArchNetwork& e) {
        throw XSocketCreate(e.what());
    }
}

ArchSocketFacade::~ArchSocketFacade()
{
    closeSocket();
}

void ArchSocketFacade::setNoDelayOnSocket()
{
    try {
        ARCH->setNoDelayOnSocket(m_socket, true);
    }
    catch(const XArchNetwork& e) {
        throw XSocketCreate(e.what());
    }
}

void ArchSocketFacade::closeSocket()
{
    if (m_socket != nullptr) {
        try {
            LOG((CLOG_DEBUG "Closing socket: %08X", m_socket));
            ARCH->closeSocket(m_socket);
        }
        catch (const XArchNetwork& e) {
            // ignore, there's not much we can do
            LOG((CLOG_WARN "error closing socket: %s", e.what()));
        }
        m_socket = nullptr;
    }
}

void ArchSocketFacade::bindSocket(const NetworkAddress& addr)
{
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

void ArchSocketFacade::closeSocketForRead()
{
    try {
        ARCH->closeSocketForRead(m_socket);
    }
    catch (const XArchNetwork& e) {
        // ignore, there's not much we can do
        LOG((CLOG_WARN "error closing socket: %s", e.what()));
    }
}

void ArchSocketFacade::closeSocketForWrite()
{
    try {
        ARCH->closeSocketForWrite(m_socket);
    }
    catch (const XArchNetwork& e) {
        // ignore, there's not much we can do
        LOG((CLOG_WARN "error closing socket: %s", e.what()));
    }
}

bool ArchSocketFacade::connectSocket(const NetworkAddress& addr)
{
    try {
        return ARCH->connectSocket(m_socket, addr.getAddress());
    }
    catch (const XArchNetwork& e) {
        throw XSocketConnect(e.what());
    }
}

size_t ArchSocketFacade::readSocket(UInt8* buffer, size_t size)
{
    return ARCH->readSocket(m_socket, buffer, size);
}

size_t ArchSocketFacade::writeSocket(const UInt8* buffer, size_t size)
{
    return ARCH->writeSocket(m_socket, buffer, size);
}

bool ArchSocketFacade::isValid() const
{
    return (m_socket != nullptr);
}

void ArchSocketFacade::throwErrorOnSocket()
{
    ARCH->throwErrorOnSocket(m_socket);
}

ArchSocket ArchSocketFacade::getRawSocket()
{
    return m_socket;
}

void ArchSocketFacade::setReuseAddrOnSocket()
{
    ARCH->setReuseAddrOnSocket(m_socket, true);
}

void ArchSocketFacade::listenOnSocket()
{
    ARCH->listenOnSocket(m_socket);
}
