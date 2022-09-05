#pragma once

#include "arch/IArchNetwork.h"
#include "NetworkAddress.h"

class ArchSocketFacade
{
public:
    ArchSocketFacade(IArchNetwork::EAddressFamily family);
    ~ArchSocketFacade();
    void setNoDelayOnSocket();
    void closeSocket();
    void bindSocket(const NetworkAddress& addr);
    void closeSocketForRead();
    void closeSocketForWrite();
    bool connectSocket(const NetworkAddress& addr);
    size_t readSocket(UInt8* buffer, size_t size);
    size_t writeSocket(const UInt8* buffer, size_t size);
    void throwErrorOnSocket();
    bool isValid() const;
    ArchSocket getRawSocket();

private:
    ArchSocket m_socket = nullptr;
};
