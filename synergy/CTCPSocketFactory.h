#ifndef CTCPSOCKETFACTORY_H
#define CTCPSOCKETFACTORY_H

#include "ISocketFactory.h"

class CTCPSocketFactory : public ISocketFactory {
  public:
	CTCPSocketFactory();
	virtual ~CTCPSocketFactory();

	// manipulators

	// accessors

	// ISocketFactory overrides
	virtual ISocket*	create() const throw(XSocket);
	virtual IListenSocket*	createListen() const throw(XSocket);
};

#endif
