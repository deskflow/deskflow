/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Bolton Software Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#import "COSXDragSimulator.h"
#import "COSXDragView.h"
#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>
#import <Cocoa/Cocoa.h>

#if defined(MAC_OS_X_VERSION_10_7)

NSWindow* g_dragWindow = NULL;
COSXDragView* g_dragView = NULL;

void
runCocoaApp()
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	
	NSApplication* app = [[NSApplication alloc] init];
    NSWindow* window = [[NSWindow alloc]
						initWithContentRect: NSMakeRect(-100, -100, 100, 100)
						styleMask: NSTitledWindowMask | NSMiniaturizableWindowMask
						backing: NSBackingStoreBuffered
						defer: NO];
    [window setTitle: @""];
	[window makeKeyAndOrderFront:nil];
	
	COSXDragView* dragView = [[COSXDragView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
	g_dragWindow = window;
	g_dragView = dragView;
	[window setContentView: dragView];
	
	[app run];
	
	[pool release];
}

void
fakeDragging(const char* str, int length, int cursorX, int cursorY)
{
	dispatch_async(dispatch_get_main_queue(), ^{
	NSRect screen = [[NSScreen mainScreen] frame];
	NSLog ( @"mouseLocation: %d %d", cursorX, cursorY);
	NSRect rect = NSMakeRect(cursorX - 50, screen.size.height - cursorY - 50, 100, 100);
	[g_dragWindow setFrame:rect display:YES];
	[g_dragWindow makeKeyWindow];
	});
}

CFStringRef
getCocoaDropTarget()
{
	return [g_dragView getDropTarget];
}

#endif
