#ifndef IPRIMARYSCREENFACTORY_H
#define IPRIMARYSCREENFACTORY_H

#include "IInterface.h"

class CPrimaryScreen;
class IPrimaryScreenReceiver;
class IScreenReceiver;

//! Primary screen factory interface
/*!
This interface provides factory methods to create primary screens.
*/
class IPrimaryScreenFactory : public IInterface {
public:
	//! Create screen
	/*!
	Create and return a primary screen.  The caller must delete the
	returned object.
	*/
	virtual CPrimaryScreen*
						create(IScreenReceiver*, IPrimaryScreenReceiver*) = 0;
};

#endif
