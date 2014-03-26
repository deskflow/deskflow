/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#pragma once

#include "common/IInterface.h"

typedef int (*StartupFunc)(int, char**);

class ILogOutputter;
class CArgsBase;
class IArchTaskBarReceiver;
class CScreen;
class IEventQueue;

class IMinimalApp : public IInterface
{
public:
	//! Returns commonly used command line args.
	virtual CArgsBase&	argsBase() const = 0;

	//! Function pointer to exit immediately.
	virtual void		setByeFunc(void(*bye)(int)) = 0;

	//! Immediately exit program.
	virtual void		bye(int error) = 0;

	//! Returns true if argv[argi] is equal to name1 or name2.
	virtual bool		isArg(int argi, int argc, const char* const* argv,
							const char* name1, const char* name2,
							int minParams = 0) = 0;
};

class IApp : public IMinimalApp
{
public:
	virtual int			standardStartup(int argc, char** argv) = 0;
	virtual int			runInner(int argc, char** argv, ILogOutputter* outputter,
							StartupFunc startup) = 0;
	virtual void		startNode() = 0;
	virtual IArchTaskBarReceiver*
						taskBarReceiver() const = 0;
	virtual int			mainLoop() = 0;
	virtual void		initApp(int argc, const char** argv) = 0;
	virtual const char*	daemonName() const = 0;
	virtual int			foregroundStartup(int argc, char** argv) = 0;
	virtual CScreen*	createScreen() = 0;
	virtual IEventQueue*
						getEvents() const = 0;
};
