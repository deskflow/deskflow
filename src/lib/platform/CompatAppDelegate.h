//
//  CompatAppDelegate.h
//  ApplicationNotification
//
//  Created by Scott Carpenter on 5/23/17.
//  Copyright © 2017 snaxco. All rights reserved.
//

#ifndef CompatAppDelegate_h
#define CompatAppDelegate_h

@interface CompatAppDelegate:NSObject
@property(nonatomic) BOOL isActive;

-(void) compatAppActive:(NSNotification *) notification;

@end


#endif /* CompatAppDelegate_h */
