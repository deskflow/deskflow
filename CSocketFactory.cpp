#include "CSocketFactory.h"
#include "BasicTypes.h"
#include <assert.h>

//
// CSocketFactory
//

CSocketFactory*			CSocketFactory::s_instance = NULL;

CSocketFactory::CSocketFactory()
{
	// do nothing
}

CSocketFactory::~CSocketFactory()
{
	// do nothing
}

void					CSocketFactory::setInstance(CSocketFactory* factory)
{
	delete s_instance;
	s_instance = factory;
}

CSocketFactory*			CSocketFactory::getInstance()
{
	assert(s_instance != NULL);
	return s_instance;
}
