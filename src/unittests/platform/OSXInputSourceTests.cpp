/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "platform/OSXKeyState.h"

#include <QEventLoop>
#include <QTest>
#include <QTimer>
#include <atomic>
#include <thread>

extern "C" int deskflowBackgroundTisCalls();
extern "C" void deskflowResetTisAudit(bool missingLayout);

class OSXInputSourceTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void pollActiveGroup_worker_returnsMappedGroup_data()
  {
    QTest::addColumn<bool>("languageSync");
    QTest::newRow("sync-disabled") << false;
    QTest::newRow("sync-enabled") << true;
  }

  void pollActiveGroup_worker_returnsMappedGroup()
  {
    QFETCH(bool, languageSync);
    Arch arch;
    arch.init();
    Log log;
    log.setFilter(LogLevel::Level::Error);
    EventQueue events;
    deskflow::KeyMap keyMap;
    OSXKeyState keys(&events, keyMap, {"en"}, languageSync);
    deskflowResetTisAudit(false);
    keys.updateKeyMap();
    QCOMPARE(keys.pollActiveGroup(), 1);

    std::atomic<int> wrongGroups{0};
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    connect(&timeout, &QTimer::timeout, [] { qFatal("input source worker timed out"); });
    timeout.start(30000);
    std::thread worker([&] {
      for (int i = 0; i < 2000; ++i) {
        if (keys.pollActiveGroup() != 1)
          ++wrongGroups;
      }
      QMetaObject::invokeMethod(&loop, &QEventLoop::quit, Qt::QueuedConnection);
    });
    loop.exec();
    worker.join();

    QCOMPARE(deskflowBackgroundTisCalls(), 0);
    QCOMPARE(wrongGroups.load(), 0);
  }

  void pollActiveGroup_missingLayout_returnsFirstGroup()
  {
    Arch arch;
    arch.init();
    Log log;
    log.setFilter(LogLevel::Level::Error);
    EventQueue events;
    deskflow::KeyMap keyMap;
    OSXKeyState keys(&events, keyMap, {"en"}, false);
    deskflowResetTisAudit(false);
    keys.updateKeyMap();
    deskflowResetTisAudit(true);

    const auto group = keys.pollActiveGroup();
    const auto backgroundCalls = deskflowBackgroundTisCalls();

    deskflowResetTisAudit(false);
    QCOMPARE(group, 0);
    QCOMPARE(backgroundCalls, 0);
  }
};

QTEST_MAIN(OSXInputSourceTests)
#include "OSXInputSourceTests.moc"
