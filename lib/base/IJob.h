#ifndef IJOB_H
#define IJOB_H

#include "IInterface.h"

//! Job interface
/*!
A job is an interface for executing some function.
*/
class IJob : public IInterface {
public:
	//! Run the job
	virtual void		run() = 0;
};

#endif
