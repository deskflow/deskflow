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

#include "CArchAppUtilUnix.h"

CArchAppUtilUnix::CArchAppUtilUnix()
{
}

CArchAppUtilUnix::~CArchAppUtilUnix()
{
}

bool 
CArchAppUtilUnix::parseArg(const int& argc, const char* const* argv, int& i)
{
#if WINAPI_XWINDOWS
	if (app().isArg(i, argc, argv, "-display", "--display", 1)) {
		// use alternative display
		app().argsBase().m_display = argv[++i];
	}

	else {
		// option not supported here
		return false;
	}

	return true;
#else
	// no options for carbon
	return false;
#endif
}

int
standardStartupStatic(int argc, char** argv)
{
	return CArchAppUtil::instance().app().standardStartup(argc, argv);
}

int
CArchAppUtilUnix::run(int argc, char** argv, CreateTaskBarReceiverFunc createTaskBarReceiver)
{
	return app().runInner(argc, argv, NULL, &standardStartupStatic, createTaskBarReceiver);
}

void
CArchAppUtilUnix::startNode()
{
	app().startNode();
}
