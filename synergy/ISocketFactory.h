#ifndef ISOCKETFACTORY_H
#define ISOCKETFACTORY_H

#include "IInterface.h"

class ISocket;
class IListenSocket;

class ISocketFactory : public IInterface {
public:
	// manipulators

	// accessors

	// create sockets
	virtual ISocket*	create() const = 0;
	virtual IListenSocket*	createListen() const = 0;
};

#endif
