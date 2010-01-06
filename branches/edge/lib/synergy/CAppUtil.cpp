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
 
#include "CAppUtil.h"
 
CAppUtil::CAppUtil() :
m_app(nullptr)
{
}

CAppUtil::~CAppUtil()
{
}

bool 
CAppUtil::parseArg(const int& argc, const char* const* argv, int& i)
{
	// no common platform args (yet)
	return false;
}

void
CAppUtil::adoptApp(CApp* app)
{
	m_app = app;
}

CApp&
CAppUtil::app() const
{
	assert(m_app != nullptr);
	return *m_app;
}