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

#import "AXDatabaseCleaner.h"

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090

#import <Cocoa/Cocoa.h>
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
#import <ServiceManagement/ServiceManagement.h>
#endif
#import <Security/Authorization.h>
#import <QMessageBox>
#import <QTime>

const NSString* const label = @"synmacph";

class AXDatabaseCleaner::Private {
public:
	NSAutoreleasePool*	autoReleasePool;
	AuthorizationRef	authRef;
	xpc_connection_t xpcConnection;
};

AXDatabaseCleaner::AXDatabaseCleaner()
{
	m_private = new Private;
	m_private->autoReleasePool = [[NSAutoreleasePool alloc] init];

	m_waitForResponse = false;
}

AXDatabaseCleaner::~AXDatabaseCleaner()
{
	[m_private->autoReleasePool release];
	delete m_private;
}

bool AXDatabaseCleaner::loadPrivilegeHelper()
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090 // mavericks

	OSStatus status = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &m_private->authRef);
	if (status != errAuthorizationSuccess) {
		assert(NO);
		m_private->authRef = NULL;
	}

	AuthorizationItem authItem = {kSMRightBlessPrivilegedHelper, 0, NULL, 0};
	AuthorizationRights authRights = {1, &authItem};
	AuthorizationFlags flags = kAuthorizationFlagDefaults
		| kAuthorizationFlagInteractionAllowed
		| kAuthorizationFlagPreAuthorize
		| kAuthorizationFlagExtendRights;

	BOOL result = NO;
	NSError* error = nil;

	status = AuthorizationCopyRights(m_private->authRef, &authRights, kAuthorizationEmptyEnvironment, flags, NULL);
	if (status != errAuthorizationSuccess) {
		error = [NSError errorWithDomain:NSOSStatusErrorDomain code:status userInfo:nil];
	}
	else {
		CFErrorRef cfError;
		result = (BOOL)SMJobBless(kSMDomainSystemLaunchd, (CFStringRef)label, m_private->authRef, &cfError);

		if (!result) {
			error = CFBridgingRelease(cfError);
		}
	}

	if (!result) {
		assert(error != nil);
		NSLog(@"bless error: domain= %@ / code= %d", [error domain], (int) [error code]);
		return false;
	}

	return true;
}

bool AXDatabaseCleaner::xpcConnect()
{
	const char *cStr = [label cStringUsingEncoding:NSASCIIStringEncoding];
	m_private->xpcConnection = xpc_connection_create_mach_service(
		cStr,
		NULL,
		XPC_CONNECTION_MACH_SERVICE_PRIVILEGED);

	if (!m_private->xpcConnection) {
		NSLog(@"failed to create xpc connection");
		return false;
	}

	xpc_connection_set_event_handler(m_private->xpcConnection, ^(xpc_object_t event) {
		xpc_type_t type = xpc_get_type(event);

		if (type == XPC_TYPE_ERROR) {
			if (event == XPC_ERROR_CONNECTION_INTERRUPTED) {
					NSLog(@"xpc connection interupted");

			}
			else if (event == XPC_ERROR_CONNECTION_INVALID) {
				NSLog(@"xpc connection invalid, releasing");
				xpc_release(m_private->xpcConnection);
			}
			else {
				NSLog(@"unexpected xpc connection error");
			}
		}
		else {
			NSLog(@"unexpected xpc connection event");
		}
	});

	xpc_connection_resume(m_private->xpcConnection);

	return true;
}

bool AXDatabaseCleaner::privilegeCommand(const char* command)
{
	xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);
	xpc_dictionary_set_string(message, "request", command);
	m_waitForResponse = true;

	xpc_connection_send_message_with_reply(
		m_private->xpcConnection,
		message,
		dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0),
		^(xpc_object_t event) {
			const char* response = xpc_dictionary_get_string(event, "reply");
			NSLog(@"reply from helper tool: %s", response);
			m_waitForResponse = false;
		});

	QTime time = QTime::currentTime();
	time.start();

	while (m_waitForResponse) {
		sleep(1);
		if (time.elapsed() > 10000) {
			QMessageBox::critical(NULL, "Synergy",
				QObject::tr("No response from helper tool.Restart Synergy may solve this problem."));
			return false;
		}
	}
#endif

	return true;
}

#endif
