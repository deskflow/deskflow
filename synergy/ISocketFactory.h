#ifndef ISOCKETFACTORY_H
#define ISOCKETFACTORY_H

#include "IInterface.h"
#include "XSocket.h"

class ISocket;
class IListenSocket;

class ISocketFactory : public IInterface {
  public:
	// manipulators

	// accessors

	// create sockets
	virtual ISocket*	create() const throw(XSocket) = 0;
	virtual IListenSocket*	createListen() const throw(XSocket) = 0;
};

#endif
