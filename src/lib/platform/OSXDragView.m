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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#import "platform/OSXDragView.h"

#ifdef MAC_OS_X_VERSION_10_7

@implementation OSXDragView

@dynamic draggingFormation;
@dynamic animatesToDestination;
@dynamic numberOfValidItemsForDrop;

- (id)
initWithFrame:(NSRect)frame
{
	self = [super initWithFrame:frame];
	m_dropTarget = [[NSMutableString alloc] initWithCapacity:0];
	m_dragFileExt = [[NSMutableString alloc] initWithCapacity:0];
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
	[self dragPromisedFilesOfTypes:[NSArray arrayWithObject:m_dragFileExt]
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

- (NSDragOperation)
draggingSourceOperationMaskForLocal:(BOOL)flag
{
	return NSDragOperationCopy;
}

- (CFStringRef)
getDropTarget
{
	NSMutableString* string;
	string = [[NSMutableString alloc] initWithCapacity:0];
	[string appendString:m_dropTarget];
	return (CFStringRef)string;
}

- (void)
clearDropTarget
{
	[m_dropTarget setString:@""];
}

- (void)
setFileExt:(NSString*) ext
{
	[ext retain];
	[m_dragFileExt release];
	m_dragFileExt = ext;
	NSLog(@"drag file ext: %@", m_dragFileExt);
}

- (NSWindow *)
draggingDestinationWindow
{
	return nil;
}

- (NSDragOperation)
draggingSourceOperationMask
{
	return NSDragOperationCopy;
}

- (NSPoint)draggingLocation
{
	NSPoint point;
	return point;
}

- (NSPoint)draggedImageLocation
{
	NSPoint point;
	return point;
}

- (NSImage *)draggedImage
{
	return nil;
}

- (NSPasteboard *)draggingPasteboard
{
	return nil;
}

- (id)draggingSource
{
	return nil;
}

- (NSInteger)draggingSequenceNumber
{
	return 0;
}

- (void)slideDraggedImageTo:(NSPoint)screenPoint
{
}

- (NSDragOperation)draggingSession:(NSDraggingSession *)session sourceOperationMaskForDraggingContext:(NSDraggingContext)context
{
	return NSDragOperationCopy;
}

- (void)enumerateDraggingItemsWithOptions:(NSDraggingItemEnumerationOptions)enumOpts forView:(NSView *)view classes:(NSArray *)classArray searchOptions:(NSDictionary *)searchOptions usingBlock:(void (^)(NSDraggingItem *draggingItem, NSInteger idx, BOOL *stop))block
{
}

@end

#endif
