#ifndef IINTERFACE_H
#define IINTERFACE_H

#include "common.h"

//! Base class of interfaces
/*!
This is the base class of all interface classes.  An interface class has
only pure virtual methods.
*/
class IInterface {
public:
	//! Interface destructor does nothing
	virtual ~IInterface() { }
};

#endif
