/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
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

#include <QMessageBox>
#include <QObject>

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

NavigationGestureDirection recordMacNavigationGesture(QWidget *parent, const QString &actionName)
{
  QMessageBox dialog(parent);
  dialog.setIcon(QMessageBox::Information);
  dialog.setWindowTitle(QObject::tr("Detect navigation gesture"));
  dialog.setText(QObject::tr("Press the Options+ action to use for %1.").arg(actionName));
  dialog.setInformativeText(
      QObject::tr(
          "Deskflow can detect horizontal or vertical macOS swipe events. Cancel if this action produces a "
          "standard key, mouse event, or an unsupported system action."
      )
  );
  dialog.setStandardButtons(QMessageBox::Cancel);

  __block NavigationGestureDirection direction = NavigationGestureDirection::None;
  QMessageBox *dialogPtr = &dialog;
  id monitor =
      [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskSwipe
                                            handler:^NSEvent *(NSEvent *event) {
                                              direction = navigationGestureDirectionFromDeltas(
                                                  static_cast<double>(event.deltaX), static_cast<double>(event.deltaY)
                                              );
                                              if (direction != NavigationGestureDirection::None) {
                                                dialogPtr->accept();
                                                return nil;
                                              }
                                              return event;
                                            }];

  dialog.exec();
  if (monitor != nil) {
    [NSEvent removeMonitor:monitor];
  }
  return direction;
}
