/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013-2016 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import "OSXCompatAppDelegate.h"

@implementation OSXCompatAppDelegate

BOOL *_compatModeActive;

-(void) registerNotifications: (BOOL*) compatModeActive
{
    _compatModeActive = compatModeActive;
    
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(compatAppActive:) name:NSWorkspaceDidActivateApplicationNotification object:nil];
}

-(void) unRegisterNotifications
{
    [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self];
    _compatModeActive = nil;
}


-(id)init
{
    self = [super init];
    
    _compatModeActive = nil;
    
    return self;
}

-(void) compatAppActive:(NSNotification *)notification
{
    @synchronized(self) {
        NSRunningApplication *runApp = [[notification userInfo] valueForKey:@"NSWorkspaceApplicationKey"];
        NSLog(@"%d: %@", *_compatModeActive, runApp.bundleIdentifier);
        if ([runApp.bundleIdentifier rangeOfString:@"com.parallels"].location != NSNotFound) {
            if (!*_compatModeActive) {
                *_compatModeActive = true;
            }
        }
        else {
            *_compatModeActive = false;
        }
    }
}

@end
