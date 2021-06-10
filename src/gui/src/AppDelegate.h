#ifndef APPDELEGATE_H
#define APPDELEGATE_H
#ifdef __cplusplus
extern "C" {
#endif

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate, NSUserNotificationCenterDelegate>

@end

#ifdef __cplusplus
}
#endif
#endif // APPDELEGATE_H
