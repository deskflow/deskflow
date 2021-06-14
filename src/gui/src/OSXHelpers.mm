/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
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

#import "OSXHelpers.h"

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>
#import <Cocoa/Cocoa.h>
#import <base/Log.h>

#import <Carbon/Carbon.h>
#import <mach/mach_port.h>
#import <mach/mach_interface.h>
#import <mach/mach_init.h>
#import <IOKit/IOMessage.h>
#import <IOKit/IOKitLib.h>
#import <libproc.h>

#import <UserNotifications/UNNotification.h>
#import <UserNotifications/UNUserNotificationCenter.h>

bool
isOSXSecureInputEnabled()
{
    return IsSecureEventInputEnabled();
}

int
getOSXSecureInputEventPID()
{
    io_service_t		service = MACH_PORT_NULL, service_root = MACH_PORT_NULL;
    mach_port_t			masterPort;

    kern_return_t kr = IOMasterPort( MACH_PORT_NULL, &masterPort );
    if(kr != KERN_SUCCESS) return 0;

    // IO registry refuses to tap into the root level directly
    // as a workaround access the parent of the top user level
    service = IORegistryEntryFromPath( masterPort, kIOServicePlane ":/" );
    IORegistryEntryGetParentEntry(service, kIOServicePlane, &service_root);

    std::unique_ptr<std::remove_pointer<CFTypeRef>::type, decltype(&CFRelease)> consoleUsers(
        IORegistryEntrySearchCFProperty(service_root, kIOServicePlane, CFSTR("IOConsoleUsers"), NULL, kIORegistryIterateParents | kIORegistryIterateRecursively),
        CFRelease
    );
    if(!consoleUsers) return 0;

    CFTypeID type = CFGetTypeID(consoleUsers.get());
    if(type != CFArrayGetTypeID()) return 0;

    CFTypeRef dict = CFArrayGetValueAtIndex((CFArrayRef)consoleUsers.get(), 0);
    if(!dict) return 0;

    type = CFGetTypeID(dict);
    if(type != CFDictionaryGetTypeID()) return 0;

    CFTypeRef secureInputPID = nullptr;
    CFDictionaryGetValueIfPresent((CFDictionaryRef)dict, CFSTR("kCGSSessionSecureInputPID"), &secureInputPID);

    if(secureInputPID == nullptr) return 0;

    type = CFGetTypeID(secureInputPID);
    if(type != CFNumberGetTypeID()) return 0;

    auto pidRef = (CFNumberRef)secureInputPID;
    CFNumberType numberType = CFNumberGetType(pidRef);
    if(numberType != kCFNumberSInt32Type) return 0;

    int pid;
    CFNumberGetValue(pidRef, kCFNumberSInt32Type, &pid);
    return pid;
}

std::string
getOSXProcessName(int pid)
{
    if(!pid) return "";
    char buf[128];
    proc_name(pid, buf, sizeof(buf));
    return buf;
}

void
OSXShowNotification()
{
    NSLog(@"Showing notification");

    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
    [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert + UNAuthorizationOptionSound)
       completionHandler:^(BOOL granted, NSError * _Nullable error) {
          // Enable or disable features based on authorization.
        NSLog(@"granted %d", granted);
    }];

    NSLog(@"Showed notification");

    NSUserNotification* notification = [[NSUserNotification alloc] init];
    notification.title = @"title";
    notification.informativeText = @"message";
    notification.soundName = NSUserNotificationDefaultSoundName;   //Will play a default sound
    [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification: notification];
    [notification autorelease];

    NSLog(@"Showed notification 2");

    /*
    if (NSClassFromString(@"UNUserNotificationCenter")) {
        UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
        UNAuthorizationOptions options = (UNAuthorizationOptionBadge | UNAuthorizationOptionAlert | UNAuthorizationOptionSound);

        [center requestAuthorizationWithOptions: options
                              completionHandler: ^(BOOL granted, NSError * _Nullable error) {
                                  if (granted) {
                                      NSLog(@"Granted notifications!");
                                  }
                              }];
    }
    else {
        NSLog(@"No notifications");
        //UIUserNotificationType userNotificationTypes = (UIUserNotificationTypeBadge | UIUserNotificationTypeAlert | UIUserNotificationTypeSound);
        //UIUserNotificationSettings *settings = [UIUserNotificationSettings settingsForTypes: userNotificationTypes categories: nil];
        //[[UIApplication sharedApplication] registerUserNotificationSettings: settings];
    }
    */
}

bool
isOSXInterfaceStyleDark()
{
   // Implementation from http://stackoverflow.com/a/26472651
   NSDictionary* dict = [[NSUserDefaults standardUserDefaults] persistentDomainForName:NSGlobalDomain];
   id style = [dict objectForKey:@"AppleInterfaceStyle"];
   return (style && [style isKindOfClass:[NSString class]] && NSOrderedSame == [style caseInsensitiveCompare:@"dark"]);
}

bool
isOSXUseDarkIcons()
{
   if (@available(macOS 11, *)) {
      return true;
   }
   else {
      return isOSXInterfaceStyleDark();
   }
}
