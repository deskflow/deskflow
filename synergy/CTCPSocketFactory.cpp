#include "CTCPSocketFactory.h"
#include "CTCPSocket.h"
#include "CTCPListenSocket.h"

//
// CTCPSocketFactory
//

CTCPSocketFactory::CTCPSocketFactory()
{
	// do nothing
}

CTCPSocketFactory::~CTCPSocketFactory()
{
	// do nothing
}

ISocket*				CTCPSocketFactory::create() const throw(XSocket)
{
	return new CTCPSocket;
}

IListenSocket*			CTCPSocketFactory::createListen() const throw(XSocket)
{
	return new CTCPListenSocket;
}
