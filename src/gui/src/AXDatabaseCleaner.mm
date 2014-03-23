/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Bolton Software Ltd.
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
#import <Cocoa/Cocoa.h>
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
#import <ServiceManagement/ServiceManagement.h>
#endif
#import <Security/Authorization.h>

const NSString* const label = @"synmacph";

class AXDatabaseCleaner::Private {
public:
	NSAutoreleasePool*	autoReleasePool;
	AuthorizationRef	authRef;
};

AXDatabaseCleaner::AXDatabaseCleaner()
{
	d = new Private;

	d->autoReleasePool = [[NSAutoreleasePool alloc] init];
}

AXDatabaseCleaner::~AXDatabaseCleaner()
{
	[d->autoReleasePool release];
	delete d;
}

void AXDatabaseCleaner::loadPrivilegeHelper()
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090 // mavericks

	OSStatus status = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &d->authRef);
	if (status != errAuthorizationSuccess) {
		assert(NO);
		d->authRef = NULL;
	}

	AuthorizationItem authItem = {kSMRightBlessPrivilegedHelper, 0, NULL, 0};
	AuthorizationRights authRights = {1, &authItem};
	AuthorizationFlags flags = kAuthorizationFlagDefaults
		| kAuthorizationFlagInteractionAllowed
		| kAuthorizationFlagPreAuthorize
		| kAuthorizationFlagExtendRights;

	BOOL result = NO;
	NSError* error = nil;

	status = AuthorizationCopyRights(d->authRef, &authRights, kAuthorizationEmptyEnvironment, flags, NULL);
	if (status != errAuthorizationSuccess) {
		error = [NSError errorWithDomain:NSOSStatusErrorDomain code:status userInfo:nil];
	}
	else {
		CFErrorRef cfError;
		result = (BOOL)SMJobBless(kSMDomainSystemLaunchd, (CFStringRef)label, d->authRef, &cfError);

		if (!result) {
			error = CFBridgingRelease(cfError);
		}
	}

	if (!result) {
		assert(error != nil);
		NSLog(@"bless error: domain= %@ / code= %d", [error domain], (int) [error code]);
	}

#endif
}
