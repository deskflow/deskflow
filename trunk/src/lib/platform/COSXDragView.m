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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#import "COSXDragView.h"

#ifdef MAC_OS_X_VERSION_10_7

@implementation COSXDragView

- (id)
initWithFrame:(NSRect)frame
{
	self = [super initWithFrame:frame];
	m_dropTarget = [[NSMutableString alloc] initWithCapacity:0];
    return self;
}

- (void)
drawRect:(NSRect)dirtyRect
{
}

- (BOOL)
acceptsFirstMouse:(NSEvent *)theEvent
{
	return YES;
}

- (void)
mouseDown:(NSEvent *)theEvent
{
	NSLog ( @"cocoa mouse down");
	NSPoint dragPosition;
	NSRect imageLocation;
	dragPosition = [self convertPoint:[theEvent locationInWindow]
							 fromView:nil];
	
	dragPosition.x -= 16;
	dragPosition.y -= 16;
	imageLocation.origin = dragPosition;
	imageLocation.size = NSMakeSize(32,32);
	[self dragPromisedFilesOfTypes:[NSArray arrayWithObject:@"zip"]
								fromRect:imageLocation
								  source:self
							   slideBack:NO
								   event:theEvent];
}

- (NSArray*)
namesOfPromisedFilesDroppedAtDestination:(NSURL *)dropDestination
{
	[m_dropTarget setString:@""];
	[m_dropTarget appendString:dropDestination.path];
	NSLog ( @"cocoa drop target: %@", m_dropTarget);
	return nil;
}

-(NSDragOperation)
draggingSourceOperationMaskForLocal:(BOOL)flag
{
	return NSDragOperationCopy;
}

-(CFStringRef)
getDropTarget
{
	NSMutableString* string;
	string = [[NSMutableString alloc] initWithCapacity:0];
	[string appendString:m_dropTarget];
	return (CFStringRef)string;
}

-(void)
clearDropTarget
{
	[m_dropTarget setString:@""];
}

@end

#endif
