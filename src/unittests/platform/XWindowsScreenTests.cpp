/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "XWindowsScreenTests.h"

#if WINAPI_XWINDOWS

void XWindowsScreenTests::initTestCase()
{
  m_arch.init();
  m_appUtil = std::make_unique<MockAppUtil>();
  m_events = std::make_unique<EventQueue>();
  m_screen = std::make_unique<XWindowsScreen>(nullptr, true, 120, m_events.get());
}

void XWindowsScreenTests::cleanupTestCase()
{
  m_screen.reset();
  m_events.reset();
}

void XWindowsScreenTests::testMonitorDetection()
{
  const auto &monitors = m_screen->getMonitors();
  QVERIFY(monitors.size() >= 1);
  qDebug() << "Detected" << monitors.size() << "monitor(s)";
  
  for (size_t i = 0; i < monitors.size(); i++) {
    const auto &monitor = monitors[i];
    qDebug() << "Monitor" << (int)i << ":" 
             << monitor.name.c_str()
             << "at" << (int)monitor.x << "," << (int)monitor.y
             << "size" << (int)monitor.width << "x" << (int)monitor.height
             << (monitor.isPrimary ? "(primary)" : "");
    
    QVERIFY(monitor.width > 0);
    QVERIFY(monitor.height > 0);
    QVERIFY(!monitor.name.empty());
  }
}

void XWindowsScreenTests::testGetMonitors()
{
  const auto &monitors = m_screen->getMonitors();
  
  for (const auto &monitor : monitors) {
    QVERIFY(monitor.x >= -10000 && monitor.x <= 10000);
    QVERIFY(monitor.y >= -10000 && monitor.y <= 10000);
    QVERIFY(monitor.width > 0);
    QVERIFY(monitor.height > 0);
    QVERIFY(monitor.width >= 640 && monitor.width <= 7680);
    QVERIFY(monitor.height >= 480 && monitor.height <= 4320);
  }
}

void XWindowsScreenTests::testTotalBounds()
{
  int32_t x, y, width, height;
  m_screen->getTotalBounds(x, y, width, height);
  QVERIFY(width > 0);
  QVERIFY(height > 0);
  qDebug() << "Total screen bounds:" << x << "," << y << width << "x" << height;
  const auto &monitors = m_screen->getMonitors();

  for (const auto &monitor : monitors) {
    QVERIFY(monitor.x >= x || (monitor.x + monitor.width) > x);
    QVERIFY(monitor.y >= y || (monitor.y + monitor.height) > y);
  }
}

void XWindowsScreenTests::testMonitorCount()
{
  const auto &monitors = m_screen->getMonitors();
  QVERIFY(monitors.size() >= 1);
  QVERIFY(monitors.size() <= 8);
}

void XWindowsScreenTests::testPrimaryMonitor()
{
  const auto &monitors = m_screen->getMonitors();
  int primaryCount = 0;
  for (const auto &monitor : monitors) {
    if (monitor.isPrimary) {
      primaryCount++;
      qDebug() << "Primary monitor:" << monitor.name.c_str();
    }
  }
  
  QCOMPARE(primaryCount, 1);
}

#else
#endif

QTEST_MAIN(XWindowsScreenTests)
