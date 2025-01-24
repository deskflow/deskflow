/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#import <Cocoa/Cocoa.h>

#ifdef MAC_OS_X_VERSION_10_7

@interface OSXDragView : NSView <NSDraggingSource, NSDraggingInfo> {
  NSMutableString *m_dropTarget;
  NSString *m_dragFileExt;
}

- (CFStringRef)getDropTarget;
- (void)clearDropTarget;
- (void)setFileExt:(NSString *)ext;

@end

#endif
