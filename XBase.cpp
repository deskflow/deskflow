#include "XBase.h"

// win32 wants a const char* argument to std::exception c'tor
#if CONFIG_PLATFORM_WIN32
#define STDEXCEPTARG ""
#endif

// default to no argument
#ifndef STDEXCEPTARG
#define STDEXCEPTARG
#endif

//
// XBase
//

XBase::XBase() : exception(STDEXCEPTARG)
{
	// do nothing
}

XBase::~XBase()
{
	// do nothing
}

const char*				XBase::what() const
{
	return getType();
}

const char*				XBase::getType() const
{
	return "XBase.h";
}

CString					XBase::format(const CString& fmt) const
{
	return fmt;
}
