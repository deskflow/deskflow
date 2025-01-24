/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#import "platform/OSXPasteboardPeeker.h"

#import <Cocoa/Cocoa.h>
#import <CoreData/CoreData.h>
#import <Foundation/Foundation.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

CFStringRef getDraggedFileURL()
{
  NSString *pbName = NSDragPboard;
  NSPasteboard *pboard = [NSPasteboard pasteboardWithName:pbName];

  NSMutableString *string;
  string = [[NSMutableString alloc] initWithCapacity:0];

  NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
  for (id file in files) {
    [string appendString:(NSString *)file];
    [string appendString:@"\0"];
  }

  return (CFStringRef)string;
}
