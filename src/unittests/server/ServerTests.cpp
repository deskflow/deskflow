/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerTests.h"

#include "../deskflow/MockEventQueue.h"
#include "deskflow/PlatformScreen.h"
#include "deskflow/Screen.h"
#include "server/BaseClientProxy.h"
#include "server/PrimaryClient.h"

#include "server/Server.h"

class ServerTestsAccess
{
public:
  static bool addClient(Server *server, BaseClientProxy *client)
  {
    return server->addClient(client);
  }

  static void setActive(Server *server, BaseClientProxy *client)
  {
    server->m_active = client;
  }

  static void detachClient(Server *server, BaseClientProxy *client, PrimaryClient *primary)
  {
    server->m_active = primary;
    server->m_clientSet.erase(client);
    server->m_clients.erase(client->getName());
  }

  static void onKeyDown(
      Server *server, KeyID key, KeyModifierMask mask, KeyButton button, const std::string &lang, const char *screens
  )
  {
    server->onKeyDown(key, mask, button, lang, screens);
  }

  static void
  onKeyRepeat(Server *server, KeyID key, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang)
  {
    server->onKeyRepeat(key, mask, count, button, lang);
  }
};

namespace {

class StubKeyState : public IKeyState
{
public:
  explicit StubKeyState(IEventQueue *events) : IKeyState(events)
  {
  }

  void updateKeyMap() override
  {
  }

  void updateKeyState() override
  {
  }

  void setHalfDuplexMask(KeyModifierMask) override
  {
  }

  void fakeKeyDown(KeyID, KeyModifierMask, KeyButton, const std::string &) override
  {
  }

  bool fakeKeyRepeat(KeyID, KeyModifierMask, int32_t, KeyButton, const std::string &) override
  {
    return false;
  }

  bool fakeKeyUp(KeyButton) override
  {
    return false;
  }

  void fakeAllKeysUp() override
  {
  }

  bool fakeCtrlAltDel() override
  {
    return false;
  }

  bool fakeMediaKey(KeyID) override
  {
    return false;
  }

  bool isKeyDown(KeyButton) const override
  {
    return false;
  }

  KeyModifierMask getActiveModifiers() const override
  {
    return 0;
  }

  KeyModifierMask pollActiveModifiers() const override
  {
    return 0;
  }

  int32_t pollActiveGroup() const override
  {
    return 0;
  }

  void pollPressedKeys(KeyButtonSet &) const override
  {
  }

  KeyID getKeyIDForButton(KeyButton button) const override
  {
    return (button == mappedButton) ? baseKey : kKeyNone;
  }

  KeyButton mappedButton = 0;
  KeyID baseKey = kKeyNone;
};

class StubPlatformScreen : public PlatformScreen
{
public:
  explicit StubPlatformScreen(IEventQueue *events) : PlatformScreen(events), m_keyState(events)
  {
  }

  void *getEventTarget() const override
  {
    return const_cast<StubPlatformScreen *>(this);
  }

  bool getClipboard(ClipboardID, IClipboard *) const override
  {
    return false;
  }

  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override
  {
    x = 0;
    y = 0;
    width = 1920;
    height = 1080;
  }

  void getCursorPos(int32_t &x, int32_t &y) const override
  {
    x = 0;
    y = 0;
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
    x = 960;
    y = 540;
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
    return true;
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

  StubKeyState m_keyState;

protected:
  void updateButtons() override
  {
  }

  IKeyState *getKeyState() const override
  {
    return const_cast<StubKeyState *>(&m_keyState);
  }

  void handleSystemEvent(const Event &) override
  {
  }
};

class RecordingClientProxy : public BaseClientProxy
{
public:
  explicit RecordingClientProxy(const std::string &name) : BaseClientProxy(name)
  {
  }

  void *getEventTarget() const override
  {
    return const_cast<RecordingClientProxy *>(this);
  }

  bool getClipboard(ClipboardID, IClipboard *) const override
  {
    return false;
  }

  void getShape(int32_t &x, int32_t &y, int32_t &width, int32_t &height) const override
  {
    x = 0;
    y = 0;
    width = 1920;
    height = 1080;
  }

  void getCursorPos(int32_t &x, int32_t &y) const override
  {
    x = 0;
    y = 0;
  }

  void enter(int32_t, int32_t, uint32_t, KeyModifierMask, bool) override
  {
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

  void keyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang) override
  {
    lastKeyDown = id;
    lastMask = mask;
    lastButton = button;
    lastLang = lang;
  }

  void keyRepeat(KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang) override
  {
    lastKeyRepeat = id;
    lastRepeatCount = count;
    lastMask = mask;
    lastButton = button;
    lastLang = lang;
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

  KeyID lastKeyDown = kKeyNone;
  KeyID lastKeyRepeat = kKeyNone;
  KeyModifierMask lastMask = 0;
  KeyButton lastButton = 0;
  int32_t lastRepeatCount = 0;
  std::string lastLang;
};

struct ServerHarness
{
  explicit ServerHarness(bool remapAltGr)
  {
    config.addScreen("server");
    config.addScreen("client");
    if (remapAltGr) {
      config.addOption("client", kOptionModifierMapForAltGr, kKeyModifierIDAlt);
    }

    platformScreen = new StubPlatformScreen(&eventQueue);
    screen = new deskflow::Screen(platformScreen, &eventQueue);
    primary = new PrimaryClient("server", screen);
    server = new Server(config, primary, screen, &eventQueue);
    client = new RecordingClientProxy("client");
    ServerTestsAccess::addClient(server, client);
    ServerTestsAccess::setActive(server, client);
  }

  ~ServerHarness()
  {
    if (server != nullptr && client != nullptr) {
      ServerTestsAccess::detachClient(server, client, primary);
      delete client;
      client = nullptr;
    }

    delete server;
    delete primary;
    delete screen;
  }

  MockEventQueue eventQueue;
  deskflow::server::Config config{&eventQueue};
  StubPlatformScreen *platformScreen = nullptr;
  deskflow::Screen *screen = nullptr;
  PrimaryClient *primary = nullptr;
  Server *server = nullptr;
  RecordingClientProxy *client = nullptr;
};

} // namespace

void ServerTests::initTestCase()
{
  m_arch.init();
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

void ServerTests::onKeyDown_altGrRemap_usesBaseKey()
{
  ServerHarness harness(true);
  harness.platformScreen->m_keyState.mappedButton = 46;
  harness.platformScreen->m_keyState.baseKey = 'l';

  ServerTestsAccess::onKeyDown(harness.server, 0x0142, KeyModifierAltGr, 46, "pl", nullptr);

  QCOMPARE(harness.client->lastKeyDown, static_cast<KeyID>('l'));
  QCOMPARE(harness.client->lastMask, KeyModifierAltGr);
  QCOMPARE(harness.client->lastButton, static_cast<KeyButton>(46));
}

void ServerTests::onKeyDown_withoutAltGrRemap_preservesTranslatedKey()
{
  ServerHarness harness(false);
  harness.platformScreen->m_keyState.mappedButton = 46;
  harness.platformScreen->m_keyState.baseKey = 'l';

  ServerTestsAccess::onKeyDown(harness.server, 0x0142, KeyModifierAltGr, 46, "pl", nullptr);

  QCOMPARE(harness.client->lastKeyDown, static_cast<KeyID>(0x0142));
  QCOMPARE(harness.client->lastMask, KeyModifierAltGr);
}

void ServerTests::onKeyRepeat_altGrRemap_usesBaseKey()
{
  ServerHarness harness(true);
  harness.platformScreen->m_keyState.mappedButton = 46;
  harness.platformScreen->m_keyState.baseKey = 'l';

  ServerTestsAccess::onKeyRepeat(harness.server, 0x0142, KeyModifierAltGr, 3, 46, "pl");

  QCOMPARE(harness.client->lastKeyRepeat, static_cast<KeyID>('l'));
  QCOMPARE(harness.client->lastRepeatCount, 3);
  QCOMPARE(harness.client->lastMask, KeyModifierAltGr);
  QCOMPARE(harness.client->lastButton, static_cast<KeyButton>(46));
}

QTEST_MAIN(ServerTests)
