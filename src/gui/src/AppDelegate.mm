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
}

-(BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification{
    return YES;
}

-(void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler{
    UNNotificationPresentationOptions presentationOptions =
    UNNotificationPresentationOptionSound
    | UNNotificationPresentationOptionAlert
    | UNNotificationPresentationOptionBadge;

    completionHandler(presentationOptions);

    [[UNUserNotificationCenter currentNotificationCenter] setDelegate:self];
}

@end
