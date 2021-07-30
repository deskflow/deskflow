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
#import <OSXNotificationDelegate.h>

OSXNotificationDelegate* notifDelegate = nil;

void requestOSXNotificationPermission(MainWindow* window)
{
#if OSX_DEPLOYMENT_TARGET >= 1014
    if (isOSXDevelopmentBuild()) {
        window->appendLogInfo("Not requesting notification permission in dev build");
        return;
    }

    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
    if(notifDelegate == nil){
        notifDelegate = [OSXNotificationDelegate new];
        center.delegate = notifDelegate;
    }
    [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert + UNAuthorizationOptionSound)
        completionHandler:^(BOOL granted, NSError * _Nullable error) {
        if(error != nil) {
            window->appendLogInfo(QString("Notification permission request error: ") + [[NSString stringWithFormat:@"%@", error] UTF8String]);
        }
    }];
#endif
}

bool
isOSXDevelopmentBuild()
{
    std::string bundleURL = [[[NSBundle mainBundle] bundleURL].absoluteString UTF8String];
    return (bundleURL.find("Synergy.app") == std::string::npos);
}

bool
showOSXNotification(MainWindow* window, const QString& title, const QString& body)
{
#if OSX_DEPLOYMENT_TARGET >= 1014
    // accessing notification center on unsigned build causes an immidiate
    // application shutodown (in this case synergys) and cannot be caught
    // to avoid issues with it need to first check if this is a dev build
    if (isOSXDevelopmentBuild()) {
        window->appendLogInfo("Not showing notification in dev build");
        return false;
    }

    requestOSXNotificationPermission(window);

    UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
    content.title = title.toNSString();
    content.body = body.toNSString();
    content.sound = [UNNotificationSound defaultSound];

    // Create the request object.
    UNNotificationRequest* request = [UNNotificationRequest requestWithIdentifier:@"SecureInput" content:content trigger:nil];

    [[UNUserNotificationCenter currentNotificationCenter] addNotificationRequest:request withCompletionHandler:^(NSError * _Nullable error) {
       if (error != nil) {
           window->appendLogInfo(QString("Notification display request error: ") + [[NSString stringWithFormat:@"%@", error] UTF8String]);
       }
    }];
#else
    NSUserNotification* notification = [[NSUserNotification alloc] init];
    notification.title = title.toNSString();
    notification.informativeText = body.toNSString();
    notification.soundName = NSUserNotificationDefaultSoundName;   //Will play a default sound
    [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification: notification];
    [notification autorelease];
#endif
    return true;
}

bool
isOSXInterfaceStyleDark()
{
   // Implementation from http://stackoverflow.com/a/26472651
   NSDictionary* dict = [[NSUserDefaults standardUserDefaults] persistentDomainForName:NSGlobalDomain];
   id style = [dict objectForKey:@"AppleInterfaceStyle"];
   return (style && [style isKindOfClass:[NSString class]] && NSOrderedSame == [style caseInsensitiveCompare:@"dark"]);
}

IconsTheme
getOSXIconsTheme()
{
   if (@available(macOS 11, *))
      return IconsTheme::ICONS_TEMPLATE;
   else if(isOSXInterfaceStyleDark())
      return IconsTheme::ICONS_DARK;
   return IconsTheme::ICONS_LIGHT;
}
