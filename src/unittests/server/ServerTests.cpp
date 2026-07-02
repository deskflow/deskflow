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
#include "common/Settings.h"
#include "deskflow/PlatformScreen.h"
#include "deskflow/Screen.h"
#include "server/Config.h"
#include "server/PrimaryClient.h"
#include "server/Server.h"

#include <QTest>

#include <memory>
#include <utility>
#include <vector>

namespace {

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
    return nullptr;
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
  std::vector<std::pair<int32_t, int32_t>> m_enterCalls;
};

} // namespace

std::unique_ptr<Arch> g_arch;

void ServerTests::initTestCase()
{
  g_arch = std::make_unique<Arch>();
  Settings::setValue(Settings::Server::MouserBridgeEnabled, false);
  Settings::setValue(Settings::Core::ComputerName, QStringLiteral("server"));
}

void ServerTests::cleanupTestCase()
{
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
  EventQueue events;
  deskflow::server::Config config(&events);
  QVERIFY(config.addScreen("server"));
  QVERIFY(config.addScreen("remote"));
  QVERIFY(config.connect("server", Direction::Right, 0.0f, 1.0f, "remote", 0.0f, 1.0f));
  QVERIFY(config.connect("remote", Direction::Left, 0.0f, 1.0f, "server", 0.0f, 1.0f));

  TestPlatformScreen platform(&events);
  deskflow::Screen screen(&platform, &events);
  PrimaryClient primary("server", &screen);
  TestClientProxy remote("remote");

  Server server(config, &primary, &screen, &events);
  server.adoptClient(&remote);
  server.switchScreen(&remote, 50, 60, false);
  remote.clearEnterLog();

  server.resyncEnterIfActiveClient(&remote);

  QCOMPARE(remote.enterCallCount(), 1);
  QCOMPARE(remote.lastEnterPos().first, 50);
  QCOMPARE(remote.lastEnterPos().second, 60);
}

void ServerTests::peekConfiguredNeighbor_returnsLinkedScreen()
{
  EventQueue events;
  deskflow::server::Config config(&events);
  QVERIFY(config.addScreen("server"));
  QVERIFY(config.addScreen("remote"));
  QVERIFY(config.connect("server", Direction::Right, 0.0f, 1.0f, "remote", 0.0f, 1.0f));

  TestPlatformScreen platform(&events);
  deskflow::Screen screen(&platform, &events);
  PrimaryClient primary("server", &screen);

  Server server(config, &primary, &screen, &events);
  int32_t x = 1023;
  int32_t y = 384;
  QCOMPARE(server.peekConfiguredNeighbor(&primary, Direction::Right, x, y), "remote");
}

QTEST_MAIN(ServerTests)
