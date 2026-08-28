/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-FileCopyrightText: (C) 2009 Sorin Sbarnea
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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

void *screenSaverUtilCreatePool()
{
  return [[NSAutoreleasePool alloc] init];
}

void screenSaverUtilReleasePool(void *pool)
{
  [(NSAutoreleasePool *)pool release];
}

void *screenSaverUtilCreateController()
{
  return [[ScreenSaverController controller] retain];
}

void screenSaverUtilReleaseController(void *controller)
{
  [(ScreenSaverController *)controller release];
}

void screenSaverUtilEnable(void *controller)
{
  [(ScreenSaverController *)controller setScreenSaverCanRun:YES];
}

void screenSaverUtilDisable(void *controller)
{
  [(ScreenSaverController *)controller setScreenSaverCanRun:NO];
}

void screenSaverUtilActivate(void *controller)
{
  [(ScreenSaverController *)controller setScreenSaverCanRun:YES];
  [(ScreenSaverController *)controller screenSaverStartNow];
}

void screenSaverUtilDeactivate(void *controller, int isEnabled)
{
  [(ScreenSaverController *)controller screenSaverStopNow];
  [(ScreenSaverController *)controller setScreenSaverCanRun:isEnabled];
}

int screenSaverUtilIsActive(void *controller)
{
  return [(ScreenSaverController *)controller screenSaverIsRunning];
}
