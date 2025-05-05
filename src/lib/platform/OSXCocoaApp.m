/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#import "platform/OSXCocoaApp.h"

#import <Cocoa/Cocoa.h>
#import <CoreData/CoreData.h>
#import <Foundation/Foundation.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

NSWindow *dummyWindow = NULL;

void runCocoaApp()
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  [NSApplication sharedApplication];

  NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 3, 3)
                                                 styleMask:NSBorderlessWindowMask
                                                   backing:NSBackingStoreBuffered
                                                     defer:NO];
  [window setTitle:@""];
  [window setAlphaValue:0.1];
  [window makeKeyAndOrderFront:nil];

  dummyWindow = window;

  NSLog(@"starting cocoa loop");
  [NSApp run];

  NSLog(@"cocoa: release");
  [pool release];
}

void stopCocoaLoop()
{
  [NSApp stop:dummyWindow];
}
