/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#import "platform/OSXScreenSaverUtil.h"

#import "platform/OSXScreenSaverControl.h"

#import <Foundation/NSAutoreleasePool.h>

//
// screenSaverUtil functions
//
// Note:  these helper functions exist only so we can avoid using ObjC++.
// autoconf/automake don't know about ObjC++ and I don't know how to
// teach them about it.
//

void*
screenSaverUtilCreatePool()
{
	return [[NSAutoreleasePool alloc] init];
}

void
screenSaverUtilReleasePool(void* pool)
{
	[(NSAutoreleasePool*)pool release];
}

void*
screenSaverUtilCreateController()
{
	return [[ScreenSaverController controller] retain];
}

void
screenSaverUtilReleaseController(void* controller)
{
	[(ScreenSaverController*)controller release];
}

void
screenSaverUtilEnable(void* controller)
{
	[(ScreenSaverController*)controller setScreenSaverCanRun:YES];
}

void
screenSaverUtilDisable(void* controller)
{
	[(ScreenSaverController*)controller setScreenSaverCanRun:NO];
}

void
screenSaverUtilActivate(void* controller)
{
	[(ScreenSaverController*)controller setScreenSaverCanRun:YES];
	[(ScreenSaverController*)controller screenSaverStartNow];
}

void
screenSaverUtilDeactivate(void* controller, int isEnabled)
{
	[(ScreenSaverController*)controller screenSaverStopNow];
	[(ScreenSaverController*)controller setScreenSaverCanRun:isEnabled];
}

int
screenSaverUtilIsActive(void* controller)
{
	return [(ScreenSaverController*)controller screenSaverIsRunning];
}
