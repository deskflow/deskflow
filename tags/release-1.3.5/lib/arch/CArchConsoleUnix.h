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

#ifndef CARCHCONSOLEUNIX_H
#define CARCHCONSOLEUNIX_H

#include "IArchConsole.h"

#define ARCH_CONSOLE CArchConsoleUnix

//! Unix implementation of IArchConsole
class CArchConsoleUnix : public IArchConsole {
public:
	CArchConsoleUnix(void*);
	virtual ~CArchConsoleUnix();

	// IArchConsole overrides
	virtual void		openConsole(const char* title);
	virtual void		closeConsole();
	virtual void		showConsole(bool);
	virtual void		writeConsole(const char*);
	virtual const char*	getNewlineForConsole();
};

#endif
