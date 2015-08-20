/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2012 Nick Bolton
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#pragma once

#include "common/IInterface.h"

typedef int (*StartupFunc)(int, char**);

class ILogOutputter;
class ArgsBase;
class IArchTaskBarReceiver;
namespace synergy { class Screen; }
class IEventQueue;

class IApp : public IInterface
{
public:
	virtual void setByeFunc(void(*bye)(int)) = 0;
	virtual ArgsBase& argsBase() const = 0;
	virtual int standardStartup(int argc, char** argv) = 0;
	virtual int runInner(int argc, char** argv, ILogOutputter* outputter, StartupFunc startup) = 0;
	virtual void startNode() = 0;
	virtual IArchTaskBarReceiver* taskBarReceiver() const = 0;
	virtual void bye(int error) = 0;
	virtual int mainLoop() = 0;
	virtual void initApp(int argc, const char** argv) = 0;
	virtual const char* daemonName() const = 0;
	virtual int foregroundStartup(int argc, char** argv) = 0;
	virtual synergy::Screen* createScreen() = 0;
	virtual IEventQueue*			getEvents() const = 0;
};
