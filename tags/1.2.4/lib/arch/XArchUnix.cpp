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

#include "XArchUnix.h"
#include <string.h>

//
// XArchEvalUnix
//

XArchEval*
XArchEvalUnix::clone() const throw()
{
	return new XArchEvalUnix(m_errno);
}

std::string
XArchEvalUnix::eval() const throw()
{
	// FIXME -- not thread safe
	return strerror(m_errno);
}
