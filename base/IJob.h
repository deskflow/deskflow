#ifndef IJOB_H
#define IJOB_H

#include "IInterface.h"

class IJob : public IInterface {
public:
	virtual void		run() = 0;
};

#endif
