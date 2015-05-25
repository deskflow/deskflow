/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Synergy Si Ltd.
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

#import "platform/OSXDragSimulator.h"

#import "platform/OSXDragView.h"

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>
#import <Cocoa/Cocoa.h>

#if defined(MAC_OS_X_VERSION_10_7)

NSWindow* g_dragWindow = NULL;
OSXDragView* g_dragView = NULL;
NSString* g_ext = NULL;

void
runCocoaApp()
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	
	[NSApplication sharedApplication];
	
    NSWindow* window = [[NSWindow alloc]
						initWithContentRect: NSMakeRect(0, 0, 3, 3)
						styleMask: NSBorderlessWindowMask
						backing: NSBackingStoreBuffered
						defer: NO];
    [window setTitle: @""];
	[window setAlphaValue:0.1];
	[window makeKeyAndOrderFront:nil];
	
	OSXDragView* dragView = [[OSXDragView alloc] initWithFrame:NSMakeRect(0, 0, 3, 3)];
	
	g_dragWindow = window;
	g_dragView = dragView;
	[window setContentView: dragView];
	
	NSLog(@"starting cocoa loop");
	[NSApp run];
	
	NSLog(@"cocoa: release");
	[pool release];
}

void
stopCocoaLoop()
{
	[NSApp stop: g_dragWindow];
}

void
fakeDragging(const char* str, int cursorX, int cursorY)
{
	g_ext = [NSString stringWithUTF8String:str];
	
	dispatch_async(dispatch_get_main_queue(), ^{
	NSRect screen = [[NSScreen mainScreen] frame];
	NSLog ( @"screen size: witdh = %f height = %f", screen.size.width, screen.size.height);
	NSLog ( @"mouseLocation: %d %d", cursorX, cursorY);
		
	int newPosX = 0;
	int newPosY = 0;
	newPosX = cursorX - 1;
	newPosY = screen.size.height - cursorY - 1;
	
	NSRect rect = NSMakeRect(newPosX, newPosY, 3, 3);
	NSLog ( @"newPosX: %d", newPosX);
	NSLog ( @"newPosY: %d", newPosY);
		
	[g_dragWindow setFrame:rect display:NO];
	[g_dragWindow makeKeyAndOrderFront:nil];
	[NSApp activateIgnoringOtherApps:YES];
	
	[g_dragView setFileExt:g_ext];

	CGEventRef down = CGEventCreateMouseEvent(CGEventSourceCreate(kCGEventSourceStateHIDSystemState), kCGEventLeftMouseDown, CGPointMake(cursorX, cursorY), kCGMouseButtonLeft);
	CGEventPost(kCGHIDEventTap, down);
	});
}

CFStringRef
getCocoaDropTarget()
{
	// HACK: sleep, wait for cocoa drop target updated first
	usleep(1000000);
	return [g_dragView getDropTarget];
}

#endif
