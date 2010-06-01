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
 
#include "CArchAppUtil.h"
#include "CApp.h"

CArchAppUtil* CArchAppUtil::s_instance = nullptr;
 
CArchAppUtil::CArchAppUtil() :
m_app(nullptr)
{
	s_instance = this;
}

CArchAppUtil::~CArchAppUtil()
{
}

bool 
CArchAppUtil::parseArg(const int& argc, const char* const* argv, int& i)
{
	// no common platform args (yet)
	return false;
}

void
CArchAppUtil::adoptApp(CApp* app)
{
	app->m_bye = &exitAppStatic;
	m_app = app;
}

CApp&
CArchAppUtil::app() const
{
	assert(m_app != nullptr);
	return *m_app;
}

CArchAppUtil&
CArchAppUtil::instance()
{
	assert(s_instance != nullptr);
	return *s_instance;
}
