/*
 * synergy -- mouse and keyboard sharing utility
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

#include "IInterface.h"

typedef int (*StartupFunc)(int, char**);

class ILogOutputter;
class CArgsBase;
class IArchTaskBarReceiver;
class CScreen;

class IApp : public IInterface
{
public:
	virtual void setByeFunc(void(*bye)(int)) = 0;
	virtual bool isArg(int argi, int argc, const char* const* argv,
		const char* name1, const char* name2,
		int minRequiredParameters = 0) = 0;
	virtual CArgsBase& argsBase() const = 0;
	virtual int standardStartup(int argc, char** argv) = 0;
	virtual int runInner(int argc, char** argv, ILogOutputter* outputter, StartupFunc startup) = 0;
	virtual void startNode() = 0;
	virtual IArchTaskBarReceiver* taskBarReceiver() const = 0;
	virtual void bye(int error) = 0;
	virtual int mainLoop() = 0;
	virtual void initApp(int argc, const char** argv) = 0;
	virtual const char* daemonName() const = 0;
	virtual int foregroundStartup(int argc, char** argv) = 0;
	virtual CScreen* createScreen() = 0;
};
