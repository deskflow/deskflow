/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "XBase.h"
#include <cerrno>
#include <cstdarg>

// win32 wants a const char* argument to std::exception c'tor
#if WINDOWS_LIKE
#include <windows.h>
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
//	exception(STDEXCEPTARG),
	m_what()
{
	// do nothing
}

XBase::XBase(const CString& msg) :
//	exception(STDEXCEPTARG),
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
#if WINDOWS_LIKE
	m_errno(GetLastError()),
#else
	m_errno(errno),
#endif
	m_string(NULL)
{
	// do nothing
}

MXErrno::MXErrno(int err) :
	m_errno(err),
	m_string(NULL)
{
	// do nothing
}

MXErrno::~MXErrno()
{
	if (m_string != NULL) {
#if WINDOWS_LIKE
		LocalFree(m_string);
#endif
	}
}

int
MXErrno::getErrno() const
{
	return m_errno;
}

const char*
MXErrno::getErrstr() const
{
#if WINDOWS_LIKE
	if (m_string != NULL) {
		if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
								FORMAT_MESSAGE_IGNORE_INSERTS |
								FORMAT_MESSAGE_FROM_SYSTEM,
								0,
								error,
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
								(LPTSTR)&m_string,
								0,
								NULL) == 0) {
			m_string = NULL;
			return "unknown error";
		}
	}
	return m_string;
#else
	return strerror(m_errno);
#endif
}
