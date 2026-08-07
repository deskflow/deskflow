/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#import "OSXHelpers.h"

#import <Cocoa/Cocoa.h>
#import <CoreData/CoreData.h>
#import <Foundation/Foundation.h>
#import <UserNotifications/UNNotification.h>
#import <UserNotifications/UNNotificationContent.h>
#import <UserNotifications/UNNotificationTrigger.h>
#import <UserNotifications/UNUserNotificationCenter.h>
#import <objc/runtime.h>

#import <QtGlobal>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace {
std::function<bool()> s_shouldQuit;
IMP s_originalShouldTerminate = nullptr;
BOOL s_isSystemShuttingDown = NO;
} // namespace

void requestOSXNotificationPermission()
{
#if OSX_DEPLOYMENT_TARGET >= 1014
  if (isOSXDevelopmentBuild()) {
    qWarning("Not requesting notification permission in dev build");
    return;
  }

  UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
  [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert + UNAuthorizationOptionSound)
                        completionHandler:^(BOOL granted, NSError *_Nullable error) {
                          if (error != nil) {
                            qWarning(
                                "Notification permission request error: %s",
                                [[NSString stringWithFormat:@"%@", error] UTF8String]
                            );
                          }
                        }];
#endif
}

bool isOSXDevelopmentBuild()
{
  std::string bundleURL = [[[NSBundle mainBundle] bundleURL].absoluteString UTF8String];
  return (bundleURL.find("Applications/Deskflow.app") == std::string::npos);
}

bool showOSXNotification(const QString &title, const QString &body)
{
#if OSX_DEPLOYMENT_TARGET >= 1014
  // accessing notification center on unsigned build causes an immidiate
  // application shutodown (in this case, server) and cannot be caught
  // to avoid issues with it need to first check if this is a dev build
  if (isOSXDevelopmentBuild()) {
    qWarning("Not showing notification in dev build");
    return false;
  }

  requestOSXNotificationPermission();

  UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];

  UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
  content.title = title.toNSString();
  content.body = body.toNSString();

  // Create the request object.
  UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@"SecureInput"
                                                                        content:content
                                                                        trigger:nil];

  [center
      addNotificationRequest:request
       withCompletionHandler:^(NSError *_Nullable error) {
         if (error != nil) {
           qWarning("Notification display request error: %s", [[NSString stringWithFormat:@"%@", error] UTF8String]);
         }
       }];
#else
  NSUserNotification *notification = [[NSUserNotification alloc] init];
  notification.title = title.toNSString();
  notification.informativeText = body.toNSString();
  notification.soundName = NSUserNotificationDefaultSoundName; // Will play a default sound
  [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];
  [notification autorelease];
#endif
  return true;
}

bool isOSXInterfaceStyleDark()
{
  // Implementation from http://stackoverflow.com/a/26472651
  NSDictionary *dict = [[NSUserDefaults standardUserDefaults] persistentDomainForName:NSGlobalDomain];
  id style = [dict objectForKey:@"AppleInterfaceStyle"];
  return (style && [style isKindOfClass:[NSString class]] && NSOrderedSame == [style caseInsensitiveCompare:@"dark"]);
}

void forceAppActive()
{
  [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
  [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
}

void macOSNativeHide()
{
  [NSApp hide:nil];
  [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyAccessory];
}

static void ignoreStatusItemMenuActivation(id, SEL, NSNotification *)
{
}

void installMacOS27TrayWorkaround()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 11, 2)
  // QTBUG-147449: macOS 27 drives status items with gesture recognizers,
  // so NSApp.currentEvent can be a non-mouse event when a tray menu opens.
  // Qt <= 6.11.1 unconditionally reads clickCount and aborts the process.
  // Deskflow does not consume tray activation signals on macOS, so suppress
  // only Qt's private menu-tracking callback until the upstream fix ships.
  if (![NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:{27, 0, 0}])
    return;

  const auto delegateClass = objc_getClass("QStatusItemDelegate");
  const auto selector = @selector(statusItemMenuBeganTracking:);
  if (const auto method = class_getInstanceMethod(delegateClass, selector); method != nullptr)
    method_setImplementation(method, reinterpret_cast<IMP>(ignoreStatusItemMenuActivation));
#endif
}

static NSApplicationTerminateReply deskflow_applicationShouldTerminate(id self, SEL _cmd, NSApplication *sender)
{
  // Don't intercept a system shutdown (or logoff/restart)
  if (!s_isSystemShuttingDown && s_shouldQuit && !s_shouldQuit()) {
    return NSTerminateCancel;
  }

  // Execute Qt's applicationShouldTerminate
  if (s_originalShouldTerminate) {
    using ShouldTerminateFn = NSApplicationTerminateReply (*)(id, SEL, NSApplication *);
    return reinterpret_cast<ShouldTerminateFn>(s_originalShouldTerminate)(self, _cmd, sender);
  }

  return NSTerminateNow;
}

void installQuitHandler(std::function<bool()> shouldQuit)
{
  s_shouldQuit = std::move(shouldQuit);

  Class cls = [[NSApp delegate] class];
  SEL selector = @selector(applicationShouldTerminate:);

  Method method = class_getInstanceMethod(cls, selector);
  if (method) {
    s_originalShouldTerminate = method_getImplementation(method);
    method_setImplementation(method, (IMP)deskflow_applicationShouldTerminate);
  } else {
    class_addMethod(cls, selector, (IMP)deskflow_applicationShouldTerminate, "l@:@");
  }

  // shutdown is also triggered for logout/restart
  [[[NSWorkspace sharedWorkspace] notificationCenter] addObserverForName:NSWorkspaceWillPowerOffNotification
                                                                  object:nil
                                                                   queue:[NSOperationQueue mainQueue]
                                                              usingBlock:^(NSNotification *note) {
                                                                Q_UNUSED(note)
                                                                s_isSystemShuttingDown = YES;
                                                              }];
}
