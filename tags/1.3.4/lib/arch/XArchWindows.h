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

#ifndef XARCHWINDOWS_H
#define XARCHWINDOWS_H

#define WIN32_LEAN_AND_MEAN

#include "XArch.h"
#include <windows.h>

//! Lazy error message string evaluation for windows
class XArchEvalWindows : public XArchEval {
public:
	XArchEvalWindows() : m_errno(GetLastError()) { }
	XArchEvalWindows(DWORD err) : m_errno(err) { }
	virtual ~XArchEvalWindows() { }

	// XArchEval overrides
	virtual XArchEval*	clone() const throw();
	virtual std::string	eval() const throw();

private:
	DWORD				m_errno;
};

//! Lazy error message string evaluation for winsock
class XArchEvalWinsock : public XArchEval {
public:
	XArchEvalWinsock(int err) : m_errno(err) { }
	virtual ~XArchEvalWinsock() { }

	// XArchEval overrides
	virtual XArchEval*	clone() const throw();
	virtual std::string	eval() const throw();

private:
	int					m_errno;
};

#endif
