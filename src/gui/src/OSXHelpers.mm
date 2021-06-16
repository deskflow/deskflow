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

#import <UserNotifications/UNNotification.h>
#import <UserNotifications/UNUserNotificationCenter.h>
#import <UserNotifications/UNNotificationContent.h>
#import <UserNotifications/UNNotificationTrigger.h>
#import <base/Log.h>
#import <QtGlobal>

void
testNotification()
{
    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
    [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert + UNAuthorizationOptionSound)
        completionHandler:^(BOOL granted, NSError * _Nullable error) {

        qWarning("granter: %d", granted);

        if(error != nil)
        {
            qWarning("error: %s", String([[NSString stringWithFormat:@"%@", error] UTF8String]).c_str());
        }

        UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
        content.title = @"Wake up!";
        content.body = @"Rise and shine! It's morning time!";

        // show after 5 seconds
        UNTimeIntervalNotificationTrigger* trigger = [UNTimeIntervalNotificationTrigger
                             triggerWithTimeInterval:(5) repeats: NO];

        // Create the request object.
        UNNotificationRequest* request = [UNNotificationRequest
               requestWithIdentifier:@"MorningAlarm" content:content trigger:trigger];

        [center addNotificationRequest:request withCompletionHandler:^(NSError * _Nullable error) {
           if (error != nil) {
               qWarning("notification: %s", String([[NSString stringWithFormat:@"%@", error] UTF8String]).c_str());
           }
        }];

    }];

    /*NSUserNotification* notification = [[NSUserNotification alloc] init];
    notification.title = @"title";
    notification.informativeText = @"message";
    notification.soundName = NSUserNotificationDefaultSoundName;   //Will play a default sound
    [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification: notification];
    [notification autorelease];*/
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
