/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2009 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// ScreenSaver.framework private API
// Class dumping by Alex Harper http://www.ragingmenace.com/

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
