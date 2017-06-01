//
//  CompatAppDelegate.m
//  ApplicationNotification
//
//  Created by Scott Carpenter on 5/23/17.
//  Copyright © 2017 snaxco. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import "CompatAppDelegate.h"

@implementation CompatAppDelegate
-(id)init
{
    self = [super init];
    
    self.isActive = false;
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(compatAppActive:) name:NSWorkspaceDidActivateApplicationNotification object:nil];
    
    return self;
}

-(void) compatAppActive:(NSNotification *)notification {
    @synchronized(self) {
        NSRunningApplication *runApp = [[notification userInfo] valueForKey:@"NSWorkspaceApplicationKey"];
        NSLog(@"%d: %@", self.isActive, runApp.bundleIdentifier);
        if ([runApp.bundleIdentifier rangeOfString:@"com.parallels"].location != NSNotFound) {
            if (!self.isActive) {
                self.isActive = true;
                NSLog(@"Enable Parallels compat mode");
            }
        }
        else {
            self.isActive = false;
            NSLog(@"Disable Parallels compat mode");
        }
    }
}

@end
