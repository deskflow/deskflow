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

#include "XArch.h"

//
// XArch
//

std::string
XArch::what() const throw()
{
	try {
		if (m_what.empty() && m_eval != NULL) {
			m_what = m_eval->eval();
		}
	}
	catch (...) {
		// ignore
	}
	return m_what;
}
