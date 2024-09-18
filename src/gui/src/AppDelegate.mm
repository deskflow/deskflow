#import "AppDelegate.h"

@interface AppDelegate ()

@property (strong) IBOutlet NSWindow *window;
@end

@implementation AppDelegate
{
}

-(void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:self];
#if OSX_DEPLOYMENT_TARGET >= 1014
    [[UNUserNotificationCenter currentNotificationCenter] setDelegate:self];
#endif
}

-(BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification{
    return YES;
}

#if OSX_DEPLOYMENT_TARGET >= 1014
-(void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler{
    UNNotificationPresentationOptions presentationOptions =
    UNNotificationPresentationOptionSound
    | UNNotificationPresentationOptionAlert
    | UNNotificationPresentationOptionBadge;

    completionHandler(presentationOptions);
}
#endif

@end
