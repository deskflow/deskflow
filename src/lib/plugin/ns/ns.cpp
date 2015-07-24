/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd
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
 */

#include "ns.h"

#include "SecureSocket.h"
#include "SecureListenSocket.h"
#include "arch/Arch.h"
#include "common/PluginVersion.h"
#include "base/Log.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <iterator>

SecureSocket* g_secureSocket = NULL;
SecureListenSocket* g_secureListenSocket = NULL;
Arch* g_arch = NULL;
Log* g_log = NULL;

std::string
helperGetLibsUsed(void)
{
	std::stringstream libs(ARCH->getLibsUsed());
	std::string msg;
	std::string pid;
	std::getline(libs,pid);

	while( std::getline(libs,msg) ) {
		LOG(( CLOG_DEBUG "libs:%s",msg.c_str()));
	}
	return pid;
}

extern "C" {
void
init(void* log, void* arch)
{
	if (g_log == NULL) {
		g_log = new Log(reinterpret_cast<Log*>(log));
	}

	if (g_arch == NULL) {
		Arch::setInstance(reinterpret_cast<Arch*>(arch));
	}

	LOG(( CLOG_DEBUG "library use: %s", helperGetLibsUsed().c_str()));
}

int
initEvent(void (*sendEvent)(const char*, void*))
{
	return 0;
}

void*
invoke(const char* command, void** args)
{
	IEventQueue* arg1 = NULL;
	SocketMultiplexer* arg2 = NULL;
	if (args != NULL) {
		arg1 = reinterpret_cast<IEventQueue*>(args[0]);
		arg2 = reinterpret_cast<SocketMultiplexer*>(args[1]);
	}

	if (strcmp(command, "getSocket") == 0) {
		if (g_secureSocket != NULL) {
			delete g_secureSocket;
		}
		g_secureSocket = new SecureSocket(arg1, arg2);
		g_secureSocket->initSsl(false);
		return g_secureSocket;
	}
	else if (strcmp(command, "getListenSocket") == 0) {
		if (g_secureListenSocket != NULL) {
			delete g_secureListenSocket;
		}
		g_secureListenSocket = new SecureListenSocket(arg1, arg2);
		return g_secureListenSocket;
	}
	else if (strcmp(command, "deleteSocket") == 0) {
		if (g_secureSocket != NULL) {
			delete g_secureSocket;
			g_secureSocket = NULL;
		}
	}
	else if (strcmp(command, "deleteListenSocket") == 0) {
		if (g_secureListenSocket != NULL) {
			delete g_secureListenSocket;
			g_secureListenSocket = NULL;
		}
	}
	else if (strcmp(command, "version") == 0) {
		return (void*)getExpectedPluginVersion(s_pluginNames[kSecureSocket]);
	}

	return NULL;
}

void
cleanup()
{
	if (g_secureSocket != NULL) {
		delete g_secureSocket;
	}

	if (g_secureListenSocket != NULL) {
		delete g_secureListenSocket;
	}
}

}
