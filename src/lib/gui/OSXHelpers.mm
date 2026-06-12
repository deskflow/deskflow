/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2015 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#import "OSXHelpers.h"

#import <Cocoa/Cocoa.h>
#import <CoreData/CoreData.h>
#import <Foundation/Foundation.h>
#import <UserNotifications/UNNotification.h>
#import <UserNotifications/UNNotificationContent.h>
#import <UserNotifications/UNNotificationTrigger.h>
#import <UserNotifications/UNUserNotificationCenter.h>

#import <QtGlobal>

#include <QAction>
#include <QIcon>
#include <QImage>
#include <QMenu>
#include <QPixmap>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace {
QString statusItemTitle(const QString &text)
{
  QString title;
  title.reserve(text.size());

  for (auto i = 0; i < text.size(); ++i) {
    if (text.at(i) != u'&') {
      title.append(text.at(i));
      continue;
    }

    if ((i + 1) < text.size() && text.at(i + 1) == u'&') {
      title.append(u'&');
      ++i;
    }
  }

  return title;
}
} // namespace

@interface DeskflowStatusItemActionTarget : NSObject {
  QAction *m_action;
}
- (instancetype)initWithAction:(QAction *)action;
- (void)trigger:(id)sender;
@end

@implementation DeskflowStatusItemActionTarget

- (instancetype)initWithAction:(QAction *)action
{
  self = [super init];
  if (self != nil) {
    m_action = action;
  }
  return self;
}

- (void)trigger:(id)sender
{
  Q_UNUSED(sender);
  if (m_action != nullptr && m_action->isEnabled()) {
    m_action->trigger();
  }
}

@end

@interface DeskflowStatusItemController : NSObject <NSMenuDelegate> {
  NSStatusItem *m_statusItem;
  NSMenu *m_menu;
  QMenu *m_qtMenu;
  NSMutableArray *m_targets;
}
- (void)setQtMenu:(QMenu *)menu;
- (void)setIcon:(const QIcon &)icon;
- (void)cleanup;
@end

@implementation DeskflowStatusItemController

- (instancetype)init
{
  self = [super init];
  if (self != nil) {
    m_statusItem = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength] retain];
    m_menu = [[NSMenu alloc] initWithTitle:@"Deskflow"];
    [m_menu setDelegate:self];
    [m_statusItem setMenu:m_menu];
    [m_statusItem button].imagePosition = NSImageOnly;
    [m_statusItem button].toolTip = @"Deskflow";
    m_targets = [[NSMutableArray alloc] init];
  }
  return self;
}

- (void)dealloc
{
  [self cleanup];
  [super dealloc];
}

- (void)setQtMenu:(QMenu *)menu
{
  m_qtMenu = menu;
}

- (void)clearMenu
{
  [m_targets removeAllObjects];
  [m_menu removeAllItems];
}

- (void)addActions:(const QList<QAction *> &)actions toMenu:(NSMenu *)menu
{
  for (auto *action : actions) {
    if (action == nullptr || !action->isVisible()) {
      continue;
    }

    if (action->isSeparator()) {
      [menu addItem:[NSMenuItem separatorItem]];
      continue;
    }

    auto *target = [[DeskflowStatusItemActionTarget alloc] initWithAction:action];
    [m_targets addObject:target];
    [target release];

    auto *item = [[NSMenuItem alloc] initWithTitle:statusItemTitle(action->text()).toNSString()
                                           action:@selector(trigger:)
                                    keyEquivalent:@""];
    [item setTarget:target];
    [item setEnabled:action->isEnabled()];

    if (action->isCheckable()) {
      [item setState:action->isChecked() ? NSControlStateValueOn : NSControlStateValueOff];
    }

    if (auto *subMenu = action->menu(); subMenu != nullptr) {
      auto *nativeSubMenu = [[NSMenu alloc] initWithTitle:statusItemTitle(subMenu->title()).toNSString()];
      [self addActions:subMenu->actions() toMenu:nativeSubMenu];
      [item setSubmenu:nativeSubMenu];
      [nativeSubMenu release];
    }

    [menu addItem:item];
    [item release];
  }
}

- (void)menuWillOpen:(NSMenu *)menu
{
  Q_UNUSED(menu);
  [self clearMenu];
  if (m_qtMenu != nullptr) {
    [self addActions:m_qtMenu->actions() toMenu:m_menu];
  }
}

- (void)setIcon:(const QIcon &)icon
{
  if (m_statusItem == nil || icon.isNull()) {
    return;
  }

  auto pixmap = icon.pixmap(QSize(22, 22));
  if (pixmap.isNull()) {
    return;
  }

  const auto image = pixmap.toImage();
  CGImageRef cgImage = image.toCGImage();
  if (cgImage == nullptr) {
    return;
  }

  const auto scale = pixmap.devicePixelRatio();
  auto *statusImage = [[NSImage alloc] initWithCGImage:cgImage
                                                 size:NSMakeSize(pixmap.width() / scale, pixmap.height() / scale)];
  [statusImage setTemplate:icon.isMask()];
  [[m_statusItem button] setImage:statusImage];
  [statusImage release];
  CGImageRelease(cgImage);
}

- (void)cleanup
{
  if (m_statusItem != nil) {
    [m_statusItem setMenu:nil];
    [[NSStatusBar systemStatusBar] removeStatusItem:m_statusItem];
    [m_statusItem release];
    m_statusItem = nil;
  }

  [m_targets release];
  m_targets = nil;

  [m_menu release];
  m_menu = nil;
  m_qtMenu = nullptr;
}

@end

static DeskflowStatusItemController *s_statusItemController = nil;

static DeskflowStatusItemController *statusItemController()
{
  if (s_statusItemController == nil) {
    s_statusItemController = [[DeskflowStatusItemController alloc] init];
  }
  return s_statusItemController;
}

void requestOSXNotificationPermission()
{
#if OSX_DEPLOYMENT_TARGET >= 1014
  if (isOSXDevelopmentBuild()) {
    qWarning("Not requesting notification permission in dev build");
    return;
  }

  UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
  [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert + UNAuthorizationOptionSound)
                        completionHandler:^(BOOL granted, NSError *_Nullable error) {
                          if (error != nil) {
                            qWarning(
                                "Notification permission request error: %s",
                                [[NSString stringWithFormat:@"%@", error] UTF8String]
                            );
                          }
                        }];
#endif
}

bool isOSXDevelopmentBuild()
{
  std::string bundleURL = [[[NSBundle mainBundle] bundleURL].absoluteString UTF8String];
  return (bundleURL.find("Applications/Deskflow.app") == std::string::npos);
}

bool showOSXNotification(const QString &title, const QString &body)
{
#if OSX_DEPLOYMENT_TARGET >= 1014
  // accessing notification center on unsigned build causes an immidiate
  // application shutodown (in this case, server) and cannot be caught
  // to avoid issues with it need to first check if this is a dev build
  if (isOSXDevelopmentBuild()) {
    qWarning("Not showing notification in dev build");
    return false;
  }

  requestOSXNotificationPermission();

  UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];

  UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
  content.title = title.toNSString();
  content.body = body.toNSString();

  // Create the request object.
  UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@"SecureInput"
                                                                        content:content
                                                                        trigger:nil];

  [center
      addNotificationRequest:request
       withCompletionHandler:^(NSError *_Nullable error) {
         if (error != nil) {
           qWarning("Notification display request error: %s", [[NSString stringWithFormat:@"%@", error] UTF8String]);
         }
       }];
#else
  NSUserNotification *notification = [[NSUserNotification alloc] init];
  notification.title = title.toNSString();
  notification.informativeText = body.toNSString();
  notification.soundName = NSUserNotificationDefaultSoundName; // Will play a default sound
  [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];
  [notification autorelease];
#endif
  return true;
}

bool isOSXInterfaceStyleDark()
{
  // Implementation from http://stackoverflow.com/a/26472651
  NSDictionary *dict = [[NSUserDefaults standardUserDefaults] persistentDomainForName:NSGlobalDomain];
  id style = [dict objectForKey:@"AppleInterfaceStyle"];
  return (style && [style isKindOfClass:[NSString class]] && NSOrderedSame == [style caseInsensitiveCompare:@"dark"]);
}

void forceAppActive()
{
  [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
  [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
}

void macOSNativeHide()
{
  [NSApp hide:nil];
  [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyAccessory];
}

void setupMacOSStatusItem(QMenu *menu)
{
  [statusItemController() setQtMenu:menu];
}

void setMacOSStatusItemIcon(const QIcon &icon)
{
  [statusItemController() setIcon:icon];
}

void cleanupMacOSStatusItem()
{
  [s_statusItemController cleanup];
  [s_statusItemController release];
  s_statusItemController = nil;
}
