#ifndef CTCPSOCKETFACTORY_H
#define CTCPSOCKETFACTORY_H

#include "ISocketFactory.h"

//! Socket factory for TCP sockets
class CTCPSocketFactory : public ISocketFactory {
public:
	CTCPSocketFactory();
	virtual ~CTCPSocketFactory();

	// ISocketFactory overrides
	virtual IDataSocket*	create() const;
	virtual IListenSocket*	createListen() const;
};

#endif
