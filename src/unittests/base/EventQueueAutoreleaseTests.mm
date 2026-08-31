/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "EventQueueAutoreleaseTests.h"

#include "base/EventQueue.h"

#import <Foundation/Foundation.h>

#include <QTest>

namespace {
bool g_deallocated = false;
} // namespace

@interface DeskflowPoolProbe : NSObject
@end

@implementation DeskflowPoolProbe
- (void)dealloc
{
  g_deallocated = true;
  [super dealloc];
}
@end

void EventQueueAutoreleaseTests::initTestCase()
{
  m_arch.init();
}

void EventQueueAutoreleaseTests::dispatchEvent_handlerAutoreleases_drainsPool()
{
  EventQueue events;
  g_deallocated = false;

  events.addHandler(EventTypes::ClientDisconnected, this, [](const Event &) {
    (void)[[[DeskflowPoolProbe alloc] init] autorelease];
  });

  QVERIFY(events.dispatchEvent(Event(EventTypes::ClientDisconnected, this)));
  QVERIFY2(g_deallocated, "autoreleased object outlived the dispatch: no pool was drained");
}

QTEST_MAIN(EventQueueAutoreleaseTests)
