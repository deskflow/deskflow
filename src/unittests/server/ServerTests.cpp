/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerTests.h"

#include "../deskflow/MockKeyState.h"
#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "common/Settings.h"
#include "deskflow/PlatformScreen.h"
#include "deskflow/Screen.h"
#include "deskflow/ipc/CoreIpcServer.h"
#include "io/IStream.h"
#include "server/Config.h"
#include "server/PrimaryClient.h"
#include "server/Server.h"
#include "server/TopologyLink.h"

#include <QCoreApplication>
#include <QTest>

#include <memory>
#include <utility>
#include <vector>

namespace {

class NullTestStream : public deskflow::IStream
{
public:
  void close() override
  {
  }
  uint32_t read(void *, uint32_t) override
  {
    return 0;
  }
  void write(const void *, uint32_t) override
  {
  }
  void flush() override
  {
  }
  void shutdownInput() override
  {
  }
  void shutdownOutput() override
  {
  }
  void *getEventTarget() const override
  {
    return nullptr;
  }
  bool isReady() const override
  {
    return false;
  }
  uint32_t getSize() const override
  {
    return 0;
  }
};

class TestPlatformScreen : public PlatformScreen
{
public:
  explicit TestPlatformScreen(IEventQueue *events) : PlatformScreen(events), m_keyState(*events)
  {
    // do nothing
  }

  void *getEventTarget() const override
  {
    return const_cast<TestPlatformScreen *>(this);
  }

  bool getClipboard(ClipboardID, IClipboard *) const override
  {
    return false;
  }

  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override
  {
    x = 0;
    y = 0;
    width = 1024;
    height = 768;
  }

  void getCursorPos(int32_t &x, int32_t &y) const override
  {
    x = 512;
    y = 384;
  }

  void reconfigure(uint32_t) override
  {
  }

  uint32_t activeSides() override
  {
    return 0;
  }

  void warpCursor(int32_t, int32_t) override
  {
  }

  uint32_t registerHotKey(KeyID, KeyModifierMask) override
  {
    return 0;
  }

  void unregisterHotKey(uint32_t) override
  {
  }

  void fakeInputBegin() override
  {
  }

  void fakeInputEnd() override
  {
  }

  int32_t getJumpZoneSize() const override
  {
    return 0;
  }

  bool isAnyMouseButtonDown(uint32_t &) const override
  {
    return false;
  }

  void getCursorCenter(int32_t &x, int32_t &y) const override
  {
    x = 512;
    y = 384;
  }

  void fakeMouseButton(ButtonID, bool) override
  {
  }

  void fakeMouseMove(int32_t, int32_t) override
  {
  }

  void fakeMouseRelativeMove(int32_t, int32_t) const override
  {
  }

  void fakeMouseWheel(ScrollDelta) const override
  {
  }

  void enable() override
  {
  }

  void disable() override
  {
  }

  void enter() override
  {
  }

  bool canLeave() override
  {
    return true;
  }

  void leave() override
  {
  }

  bool setClipboard(ClipboardID, const IClipboard *) override
  {
    return false;
  }

  void checkClipboards() override
  {
  }

  void openScreensaver(bool) override
  {
  }

  void closeScreensaver() override
  {
  }

  void screensaver(bool) override
  {
  }

  void resetOptions() override
  {
  }

  void setOptions(const OptionsList &) override
  {
  }

  void setSequenceNumber(uint32_t) override
  {
  }

  std::string getSecureInputApp() const override
  {
    return {};
  }

  bool isPrimary() const override
  {
    return true;
  }

protected:
  void updateButtons() override
  {
  }

  IKeyState *getKeyState() const override
  {
    return const_cast<MockKeyState *>(&m_keyState);
  }

  void handleSystemEvent(const Event &) override
  {
  }

private:
  MockKeyState m_keyState;
};

class TestClientProxy : public BaseClientProxy
{
public:
  explicit TestClientProxy(std::string name) : BaseClientProxy(std::move(name))
  {
  }

  void *getEventTarget() const override
  {
    return const_cast<TestClientProxy *>(this);
  }

  bool getClipboard(ClipboardID, IClipboard *) const override
  {
    return false;
  }

  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override
  {
    x = 0;
    y = 0;
    width = 1024;
    height = 768;
  }

  void getCursorPos(int32_t &x, int32_t &y) const override
  {
    x = 100;
    y = 200;
  }

  void enter(int32_t x, int32_t y, uint32_t, KeyModifierMask, bool) override
  {
    m_enterCalls.emplace_back(x, y);
  }

  bool leave() override
  {
    return true;
  }

  void setClipboard(ClipboardID, const IClipboard *) override
  {
  }

  void grabClipboard(ClipboardID) override
  {
  }

  void setClipboardDirty(ClipboardID, bool) override
  {
  }

  void keyDown(KeyID, KeyModifierMask, KeyButton, const std::string &) override
  {
  }

  void keyRepeat(KeyID, KeyModifierMask, int32_t, KeyButton, const std::string &) override
  {
  }

  void keyUp(KeyID, KeyModifierMask, KeyButton) override
  {
  }

  void mouseDown(ButtonID) override
  {
  }

  void mouseUp(ButtonID) override
  {
  }

  void mouseMove(int32_t, int32_t) override
  {
  }

  void mouseRelativeMove(int32_t, int32_t) override
  {
  }

  void mouseWheel(int32_t, int32_t) override
  {
  }

  void screensaver(bool) override
  {
  }

  void resetOptions() override
  {
  }

  void setOptions(const OptionsList &) override
  {
  }

  void sendDragInfo(uint32_t, const char *, size_t) override
  {
  }

  void fileChunkSending(uint8_t, char *, size_t) override
  {
  }

  std::string getSecureInputApp() const override
  {
    return {};
  }

  void secureInputNotification(const std::string &) const override
  {
  }

  deskflow::IStream *getStream() const override
  {
    return const_cast<NullTestStream *>(&m_stream);
  }

  void clearEnterLog()
  {
    m_enterCalls.clear();
  }

  int enterCallCount() const
  {
    return static_cast<int>(m_enterCalls.size());
  }

  std::pair<int32_t, int32_t> lastEnterPos() const
  {
    return m_enterCalls.back();
  }

private:
  NullTestStream m_stream;
  std::vector<std::pair<int32_t, int32_t>> m_enterCalls;
};

struct LeakedServerFixture
{
  EventQueue events;
  deskflow::server::Config config;
  TestPlatformScreen *platform = nullptr;
  deskflow::Screen *screen = nullptr;
  PrimaryClient *primary = nullptr;

  explicit LeakedServerFixture() : config(&events) {}

  void init(const char *primaryName)
  {
    platform = new TestPlatformScreen(&events);
    screen = new deskflow::Screen(platform, &events);
    primary = new PrimaryClient(primaryName, screen);
  }
};

} // namespace

std::unique_ptr<Arch> g_arch;
Log g_log;
std::unique_ptr<QCoreApplication> g_app;
std::unique_ptr<deskflow::core::ipc::CoreIpcServer> g_ipc;

void ServerTests::initTestCase()
{
  static int argc = 1;
  static char arg0[] = "ServerTests";
  static char *argv[] = {arg0, nullptr};
  g_app = std::make_unique<QCoreApplication>(argc, argv);
  g_ipc = std::make_unique<deskflow::core::ipc::CoreIpcServer>(g_app.get());
  g_arch = std::make_unique<Arch>();
  g_log.setFilter(LogLevel::Level::Error);
  Settings::setValue(Settings::Server::MouserBridgeEnabled, false);
  Settings::setValue(Settings::Core::ComputerName, QStringLiteral("server"));
}

void ServerTests::cleanupTestCase()
{
  g_ipc.reset();
  g_app.reset();
  g_arch.reset();
}

void ServerTests::SwitchToScreenInfo_alloc_screen()
{
  auto actual = new Server::SwitchToScreenInfo("test");
  QCOMPARE(actual->m_screen, "test");
  delete actual;
}

void ServerTests::KeyboardBroadcastInfo_alloc_stateAndSceens()
{
  auto info = new Server::KeyboardBroadcastInfo(Server::KeyboardBroadcastInfo::State::kOn, "test");
  QCOMPARE(info->m_state, Server::KeyboardBroadcastInfo::State::kOn);
  QCOMPARE(info->m_screens, "test");
  delete info;
}

void ServerTests::adoptClient_resyncsEnterWhenActiveMatches()
{
  LeakedServerFixture fixture;
  QVERIFY(fixture.config.addScreen("server"));
  QVERIFY(fixture.config.addScreen("remote"));
  QVERIFY(fixture.config.connect("server", Direction::Right, 0.0f, 1.0f, "remote", 0.0f, 1.0f));
  QVERIFY(fixture.config.connect("remote", Direction::Left, 0.0f, 1.0f, "server", 0.0f, 1.0f));
  fixture.init("server");
  TestClientProxy remote("remote");

  {
    Server server(fixture.config, fixture.primary, fixture.screen, &fixture.events);
    QVERIFY(server.m_clients.emplace("remote", &remote).second);
    server.switchScreen(&remote, 50, 60, false);
    remote.clearEnterLog();

    server.resyncEnterIfActiveClient(&remote);

    QCOMPARE(remote.enterCallCount(), 1);
    QCOMPARE(remote.lastEnterPos().first, 50);
    QCOMPARE(remote.lastEnterPos().second, 60);
    server.m_clients.erase("remote");
  }
}

void ServerTests::peekConfiguredNeighbor_returnsLinkedScreen()
{
  LeakedServerFixture fixture;
  QVERIFY(fixture.config.addScreen("server"));
  QVERIFY(fixture.config.addScreen("remote"));
  QVERIFY(fixture.config.connect("server", Direction::Right, 0.0f, 1.0f, "remote", 0.0f, 1.0f));
  fixture.init("server");

  {
    Server server(fixture.config, fixture.primary, fixture.screen, &fixture.events);
    int32_t x = 1023;
    int32_t y = 384;
    QCOMPARE(server.peekConfiguredNeighbor(fixture.primary, Direction::Right, x, y), "remote");
  }
}

void ServerTests::peekConfiguredNeighbor_usesFleetTopology()
{
  LeakedServerFixture fixture;
  QVERIFY(fixture.config.addScreen("server"));
  QVERIFY(fixture.config.addScreen("remote"));
  fixture.init("server");

  {
    Server server(fixture.config, fixture.primary, fixture.screen, &fixture.events);
    server.setFleetTopologySource(true);
    server.setFleetTopologyLinks(
        {deskflow::server::TopologyLink{"server", "remote", Direction::Right}}
    );

    int32_t x = 1023;
    int32_t y = 384;
    QCOMPARE(server.peekConfiguredNeighbor(fixture.primary, Direction::Right, x, y), "remote");
    QCOMPARE(server.peekConfiguredNeighbor(fixture.primary, Direction::Left, x, y), std::string());
  }
}

void ServerTests::queuedSwitch_executesWhenNeighborConnects()
{
  LeakedServerFixture fixture;
  QVERIFY(fixture.config.addScreen("server"));
  QVERIFY(fixture.config.addScreen("remote"));
  QVERIFY(fixture.config.connect("server", Direction::Right, 0.0f, 1.0f, "remote", 0.0f, 1.0f));
  fixture.init("server");
  TestClientProxy remote("remote");

  {
    Server server(fixture.config, fixture.primary, fixture.screen, &fixture.events);
    server.switchScreen(fixture.primary, 512, 384, false);
    server.queueSwitchForScreen("remote", Direction::Right, 1020, 384);
    QVERIFY(server.m_clients.emplace("remote", &remote).second);
    QCOMPARE(server.m_active, fixture.primary);

    server.tryExecuteQueuedSwitch(&remote);

    QCOMPARE(server.m_active, &remote);
    server.m_clients.erase("remote");
  }
}

QTEST_MAIN(ServerTests)
