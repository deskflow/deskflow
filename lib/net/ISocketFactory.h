#ifndef ISOCKETFACTORY_H
#define ISOCKETFACTORY_H

#include "IInterface.h"

class IDataSocket;
class IListenSocket;

//! Socket factory
/*!
This interface defines the methods common to all factories used to
create sockets.
*/
class ISocketFactory : public IInterface {
public:
	//! @name accessors
	//@{

	//! Create data socket
	virtual IDataSocket*	create() const = 0;

	//! Create listen socket
	virtual IListenSocket*	createListen() const = 0;

	//@}
};

#endif
