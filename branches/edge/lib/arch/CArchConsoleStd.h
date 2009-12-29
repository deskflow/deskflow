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

#pragma once

#include "IArchConsole.h"

//! Cross platform implementation of IArchConsole
class CArchConsoleStd : public IArchConsole {
public:
	CArchConsoleStd() { }
	virtual ~CArchConsoleStd() { }

	// IArchConsole overrides
	virtual void		openConsole(const char* title) { }
	virtual void		closeConsole() { }
	virtual void		showConsole(bool) { }
	virtual void		writeConsole(const char*);
};
