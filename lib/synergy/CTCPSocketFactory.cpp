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

IDataSocket*
CTCPSocketFactory::create() const
{
	return new CTCPSocket;
}

IListenSocket*
CTCPSocketFactory::createListen() const
{
	return new CTCPListenSocket;
}
