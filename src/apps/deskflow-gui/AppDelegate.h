#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

#import <Cocoa/Cocoa.h>
#if OSX_DEPLOYMENT_TARGET >= 1014
#import <UserNotifications/UNUserNotificationCenter.h>
  @interface AppDelegate
      : NSObject <NSApplicationDelegate, NSUserNotificationCenterDelegate, UNUserNotificationCenterDelegate>
#else
@interface AppDelegate : NSObject <NSApplicationDelegate, NSUserNotificationCenterDelegate>
#endif

  @end

#ifdef __cplusplus
}
#endif
