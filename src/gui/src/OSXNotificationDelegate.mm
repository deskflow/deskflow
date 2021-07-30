/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
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

#import "OSXNotificationDelegate.h"
#import <thread>
#import <QtGlobal>

@implementation OSXNotificationDelegate
- (void)userNotificationCenter:(UNUserNotificationCenter *)center
didReceiveNotificationResponse:(UNNotificationResponse *)response
         withCompletionHandler:(void (^)())completionHandler {
    completionHandler();
}
- (void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions))completionHandler {
    CFOptionFlags responseFlags = 0;
    CFUserNotificationDisplayAlert(5.0, kCFUserNotificationCautionAlertLevel, NULL, NULL, NULL,
                                   (__bridge CFStringRef)notification.request.content.title,
                                   (__bridge CFStringRef)notification.request.content.body,
                                   NULL, NULL, NULL,
                                   &responseFlags);

    completionHandler(UNNotificationPresentationOptionList | UNNotificationPresentationOptionBanner);
}
@end

