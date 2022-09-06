#pragma once

#include "arch/IArchNetwork.h"
#include "NetworkAddress.h"

class ArchSocketFacade
{
public:
    ArchSocketFacade(IArchNetwork::EAddressFamily family);
    ~ArchSocketFacade();

    void setNoDelayOnSocket();
    void setReuseAddrOnSocket();

    void closeSocket();
    void closeSocketForRead();
    void closeSocketForWrite();

    void listenOnSocket();
    ArchSocket acceptSocket();
    void bindSocket(const NetworkAddress& addr);
    bool connectSocket(const NetworkAddress& addr);

    size_t readSocket(UInt8* buffer, size_t size);
    size_t writeSocket(const UInt8* buffer, size_t size);

    bool isValid() const;
    void throwErrorOnSocket();
    ArchSocket getRawSocket();
    void operator =(ArchSocket socket);

private:
    ArchSocket m_socket = nullptr;
};
