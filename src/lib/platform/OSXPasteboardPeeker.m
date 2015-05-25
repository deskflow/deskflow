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

#import "platform/OSXPasteboardPeeker.h"

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>
#import <Cocoa/Cocoa.h>

CFStringRef
getDraggedFileURL()
{
	NSString* pbName = NSDragPboard;
	NSPasteboard* pboard = [NSPasteboard pasteboardWithName:pbName];
	
	NSMutableString* string;
	string = [[NSMutableString alloc] initWithCapacity:0];

	NSArray* files = [pboard propertyListForType:NSFilenamesPboardType];
	for (id file in files) {
		[string appendString: (NSString*)file];
		[string appendString: @"\0"];
	}
	
	return (CFStringRef)string;
}
