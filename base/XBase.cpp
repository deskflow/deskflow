#include "XBase.h"
#include <cerrno>
#include <cstdarg>

// win32 wants a const char* argument to std::exception c'tor
#if WINDOWS_LIKE
#define STDEXCEPTARG ""
#endif

// default to no argument
#ifndef STDEXCEPTARG
#define STDEXCEPTARG
#endif

//
// XBase
//

XBase::XBase() :
	exception(STDEXCEPTARG),
	m_what()
{
	// do nothing
}

XBase::XBase(const CString& msg) :
	exception(STDEXCEPTARG),
	m_what(msg)
{
	// do nothing
}

XBase::~XBase()
{
	// do nothing
}

const char*
XBase::what() const
{
	if (m_what.empty()) {
		m_what = getWhat();
	}
	return m_what.c_str();
}

CString
XBase::format(const char* /*id*/, const char* fmt, ...) const throw()
{
	// FIXME -- lookup message string using id as an index.  set
	// fmt to that string if it exists.

	// format
	CString result;
	va_list args;
	va_start(args, fmt);
	try {
		result = CStringUtil::vformat(fmt, args);
	}
	catch (...) {
		// ignore
	}
	va_end(args);

	return result;
}


//
// MXErrno
//

MXErrno::MXErrno() :
	m_errno(errno)
{
	// do nothing
}

MXErrno::MXErrno(int err) :
	m_errno(err)
{
	// do nothing
}

int
MXErrno::getErrno() const
{
	return m_errno;
}

const char*
MXErrno::getErrstr() const
{
	return strerror(m_errno);
}
