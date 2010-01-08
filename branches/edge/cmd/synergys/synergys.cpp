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

#include "CServerApp.h"

#if WINAPI_MSWINDOWS
#include "CMSWindowsServerTaskBarReceiver.h"
#elif WINAPI_XWINDOWS
#include "CXWindowsServerTaskBarReceiver.h"
#elif WINAPI_CARBON
#include "COSXServerTaskBarReceiver.h"
#else
#error Platform not supported.
#endif

int
main(int argc, char** argv) 
{
	CServerApp app;
	return app.run(argc, argv, createTaskBarReceiver);
}
