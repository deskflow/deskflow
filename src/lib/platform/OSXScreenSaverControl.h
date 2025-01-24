/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2009 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// ScreenSaver.framework private API
// Class dumping by Alex Harper http://www.ragingmenace.com/
#pragma once

#import <Foundation/NSObject.h>

@protocol ScreenSaverControl
- (double)screenSaverTimeRemaining;
- (void)restartForUser:fp12;
- (void)screenSaverStopNow;
- (void)screenSaverStartNow;
- (void)setScreenSaverCanRun:(char)fp12;
- (BOOL)screenSaverCanRun;
- (BOOL)screenSaverIsRunning;
@end

@interface ScreenSaverController : NSObject <ScreenSaverControl>

+ controller;
+ monitor;
+ daemonConnectionName;
+ daemonPath;
+ enginePath;
- init;
- (void)dealloc;
- (void)_connectionClosed:fp12;
- (BOOL)screenSaverIsRunning;
- (BOOL)screenSaverCanRun;
- (void)setScreenSaverCanRun:(char)fp12;
- (void)screenSaverStartNow;
- (void)screenSaverStopNow;
- (void)restartForUser:fp12;
- (double)screenSaverTimeRemaining;

@end
