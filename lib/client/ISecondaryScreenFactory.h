#ifndef ISECONDARYSCREENFACTORY_H
#define ISECONDARYSCREENFACTORY_H

#include "IInterface.h"

class CSecondaryScreen;
class IScreenReceiver;

//! Secondary screen factory interface
/*!
This interface provides factory methods to create secondary screens.
*/
class ISecondaryScreenFactory : public IInterface {
public:
	//! Create screen
	/*!
	Create and return a secondary screen.  The caller must delete the
	returned object.
	*/
	virtual CSecondaryScreen*
						create(IScreenReceiver*) = 0;
};

#endif
