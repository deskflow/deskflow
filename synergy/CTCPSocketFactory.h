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
	virtual IDataSocket*	create() const;
	virtual IListenSocket*	createListen() const;
};

#endif
