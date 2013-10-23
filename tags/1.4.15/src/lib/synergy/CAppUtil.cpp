/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include "CAppUtil.h"

CAppUtil* CAppUtil::s_instance = nullptr;
 
CAppUtil::CAppUtil() :
m_app(nullptr)
{
	s_instance = this;
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
CAppUtil::adoptApp(IApp* app)
{
	app->setByeFunc(&exitAppStatic);
	m_app = app;
}

IApp&
CAppUtil::app() const
{
	assert(m_app != nullptr);
	return *m_app;
}

CAppUtil&
CAppUtil::instance()
{
	assert(s_instance != nullptr);
	return *s_instance;
}
