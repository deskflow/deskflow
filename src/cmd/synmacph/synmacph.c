/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si Ltd.
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

#include <syslog.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <xpc/xpc.h>

const char* const label = "synmacph";

static void xpcEventHandler(xpc_connection_t connection, xpc_object_t event);
static void xpcConnectionHandler(xpc_connection_t connection);

static void xpcEventHandler(xpc_connection_t connection, xpc_object_t event)
{
	syslog(LOG_NOTICE, "received event in helper");

	xpc_type_t type = xpc_get_type(event);
    
	if (type == XPC_TYPE_ERROR) {
		if (event == XPC_ERROR_CONNECTION_INVALID) {
			// the client process on the other end of the connection has either
			// crashed or cancelled the connection. After receiving this error,
			// the connection is in an invalid state, and you do not need to
			// call xpc_connection_cancel(). Just tear down any associated state
			// here.
		}
		else if (event == XPC_ERROR_TERMINATION_IMMINENT) {
			// handle per-connection termination cleanup.
		}
	}
	else {
		xpc_connection_t remote = xpc_dictionary_get_remote_connection(event);

		const char* command = xpc_dictionary_get_string(event, "request");
		syslog(LOG_NOTICE, "received command in helper: %s", command);
		system(command);
		
		xpc_object_t reply = xpc_dictionary_create_reply(event);
		xpc_dictionary_set_string(reply, "reply", "command has been executed");
		xpc_connection_send_message(remote, reply);
		xpc_release(reply);
	}
}

static void xpcConnectionHandler(xpc_connection_t connection)
{
	syslog(LOG_NOTICE, "configuring message event handler for helper");

	xpc_connection_set_event_handler(connection, ^(xpc_object_t event) {
		xpcEventHandler(connection, event);
	});

	xpc_connection_resume(connection);
}

int main(int argc, const char * argv[])
{
#pragma unused(argc)
#pragma unused(argv)
	
	xpc_connection_t service = xpc_connection_create_mach_service(
		label,
		dispatch_get_main_queue(),
		XPC_CONNECTION_MACH_SERVICE_LISTENER);
	
	if (!service) {
		syslog(LOG_NOTICE, "failed to create service");
		exit(EXIT_FAILURE);
	}
	
	syslog(LOG_NOTICE, "configuring connection event handler for helper");
	xpc_connection_set_event_handler(service, ^(xpc_object_t connection) {
		xpcConnectionHandler(connection);
	});
	
	xpc_connection_resume(service);
	
	dispatch_main();

	xpc_release(service);
	
	return EXIT_SUCCESS;
}
