/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd
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

#include "ns.h"

#include "SecureSocket.h"

#include <iostream>

SecureSocket* g_secureSocket = NULL;

extern "C" {

int
init(void (*sendEvent)(const char*, void*), void (*log)(const char*))
{
	return 0;
}

void*
invoke(const char* command, void* args)
{
	if (strcmp(command, "getSecureSocket") == 0) {
		if (g_secureSocket == NULL) {
			g_secureSocket = new SecureSocket();
		}
		return g_secureSocket;
	}
	else {
		return NULL;
	}
}

int
cleanup()
{
	if (g_secureSocket != NULL) {
		delete g_secureSocket;
	}

	return 0;
}

}