#ifndef ISOCKETFACTORY_H
#define ISOCKETFACTORY_H

#include "IInterface.h"

class IDataSocket;
class IListenSocket;

class ISocketFactory : public IInterface {
public:
	// manipulators

	// accessors

	// create sockets
	virtual IDataSocket*	create() const = 0;
	virtual IListenSocket*	createListen() const = 0;
};

#endif
