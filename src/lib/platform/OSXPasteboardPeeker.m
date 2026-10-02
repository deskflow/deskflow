/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Synergy App Ltd
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
  @autoreleasepool {
    NSString *result = @"";
    @try {
      NSString *pbName = NSDragPboard;
      NSPasteboard *pboard = [NSPasteboard pasteboardWithName:pbName];

      NSMutableString *string = [[NSMutableString alloc] initWithCapacity:0];

      NSArray *types = [pboard types];
      fprintf(stderr, "[DDRG] NSDragPboard types: %lu\n", (unsigned long)[types count]);
      for (id t in types) {
        fprintf(stderr, "[DDRG]   type: %s\n", [[t description] UTF8String]);
      }

      // Try NSFilenamesPboardType first (array of paths).
      NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
      fprintf(stderr, "[DDRG] NSFilenamesPboardType count: %lu\n", (unsigned long)[files count]);
      for (id file in files) {
        fprintf(stderr, "[DDRG]   file: %s\n", [[file description] UTF8String]);
        [string appendString:(NSString *)file];
        [string appendString:@"\0"];
      }

      // Fallback: public.file-url (single URL). Finder drags often carry a
      // security-scoped bookmark (.file/id=...) that cannot be resolved to a
      // filesystem path — skip it rather than crash (the old code called
      // [url path] on it, which can throw on non-resolvable bookmarks).
      if ([files count] == 0) {
        NSString *urlStr = [pboard stringForType:@"public.file-url"];
        fprintf(stderr, "[DDRG] public.file-url: %s\n", [urlStr UTF8String]);
        if (urlStr != nil) {
          NSURL *url = [NSURL URLWithString:urlStr];
          NSString *resolvedPath = nil;
          if (url != nil && ![[url path] hasPrefix:@"/.file/id="]) {
            resolvedPath = [url path];
          }
          if (resolvedPath != nil && resolvedPath.length > 0) {
            [string appendString:resolvedPath];
            [string appendString:@"\0"];
          }
        }
      }

      result = [NSString stringWithString:string];
    } @catch (NSException *e) {
      fprintf(stderr, "[DDRG] pasteboard read failed: %s\n", [[e description] UTF8String]);
      result = @"";
    }

    const char *utf8 = [result UTF8String];
    if (utf8 == NULL) return NULL;
    // Explicit +1 copy: caller owns it and must CFRelease. Independent of ARC.
    return CFStringCreateWithCString(kCFAllocatorDefault, utf8, kCFStringEncodingUTF8);
  }
}
