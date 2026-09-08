/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/EventQueueTimer.h"
#include "client/Client.h"
#include "client/ServerProxy.h"
#include "deskflow/AppUtil.h"
#include "deskflow/PlatformScreen.h"
#include "deskflow/ProtocolUtil.h"
#include "deskflow/Screen.h"
#include "deskflow/SharedInputLockEvent.h"
#include "io/IStream.h"
#include "net/ISocketFactory.h"
#include "server/ClientProxy1_9.h"
#include "server/PrimaryClient.h"
#include "server/Server.h"
#include "unittests/legacytests/mock/deskflow/MockEventQueue.h"
#include "unittests/legacytests/mock/deskflow/MockKeyState.h"
#include <cstring>

#include <deque>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::_;
using testing::NiceMock;
using testing::Return;

namespace {

// FIFO and immediate delivery match the production event queue contract, but
// there is no real timer, screen, socket, or input injection in this fixture.
class TestQueue : public NiceMock<MockEventQueue>
{
public:
  TestQueue()
  {
    ON_CALL(*this, addHandler(_, _, _)).WillByDefault([this](auto type, auto target, const auto &handler) {
      handlers[{type, target}] = handler;
    });
    ON_CALL(*this, removeHandler(_, _)).WillByDefault([this](auto type, auto target) {
      handlers.erase({type, target});
    });
    ON_CALL(*this, dispatchEvent(_)).WillByDefault([this](const auto &event) {
      const auto found = handlers.find({event.getType(), event.getTarget()});
      if (found == handlers.end()) {
        return false;
      }
      const auto handler = found->second;
      handler(event);
      return true;
    });
    ON_CALL(*this, addEvent(_)).WillByDefault([this](Event &&event) {
      if (event.getFlags() & Event::EventFlags::DeliverImmediately) {
        dispatchEvent(event);
        Event::deleteData(event);
      } else {
        pending.push_back(std::move(event));
      }
    });
    ON_CALL(*this, newTimer(_, _)).WillByDefault([](auto, auto) { return new EventQueueTimer; });
    ON_CALL(*this, newOneShotTimer(_, _)).WillByDefault([](auto, auto) { return new EventQueueTimer; });
    ON_CALL(*this, deleteTimer(_)).WillByDefault([](auto timer) { delete timer; });
  }

  void drain()
  {
    while (!pending.empty()) {
      auto event = std::move(pending.front());
      pending.pop_front();
      dispatchEvent(event);
      Event::deleteData(event);
    }
  }

  ~TestQueue() override
  {
    for (const auto &event : pending) {
      Event::deleteData(event);
    }
  }

  std::map<std::pair<EventTypes, void *>, EventHandler> handlers;
  std::deque<Event> pending;
};

class TestPlatform : public PlatformScreen
{
public:
  explicit TestPlatform(TestQueue &queue) : PlatformScreen(&queue), keys(queue)
  {
  }

  void *getEventTarget() const override
  {
    return const_cast<TestPlatform *>(this);
  }
  bool isPrimary() const override
  {
    return true;
  }
  bool isInputBlocked() const override
  {
    return blocked;
  }
  void requestInputLock(unsigned) override
  {
    ++requests;
  }
  void confirmInputLock(uint64_t generation, bool success) override
  {
    confirmations.emplace_back(generation, success);
  }
  void resumeInput(uint64_t generation) override
  {
    resumes.push_back(generation);
    blocked = false;
  }
  void getShape(int32_t &x, int32_t &y, int32_t &w, int32_t &h) const override
  {
    x = y = 0;
    w = 1200;
    h = 800;
  }
  void getCursorPos(int32_t &x, int32_t &y) const override
  {
    x = 600;
    y = 400;
  }
  void getCursorCenter(int32_t &x, int32_t &y) const override
  {
    getCursorPos(x, y);
  }
  bool canLeave() override
  {
    return true;
  }
  bool getClipboard(ClipboardID, IClipboard *) const override
  {
    return false;
  }
  bool setClipboard(ClipboardID, const IClipboard *) override
  {
    return false;
  }
  bool isAnyMouseButtonDown(uint32_t &) const override
  {
    return false;
  }
  std::string getSecureInputApp() const override
  {
    return "";
  }
  uint32_t activeSides() override
  {
    return 0;
  }
  uint32_t registerHotKey(KeyID, KeyModifierMask) override
  {
    return ++hotkeyID;
  }
  int32_t getJumpZoneSize() const override
  {
    return 1;
  }
  void reconfigure(uint32_t) override
  {
  }
  void warpCursor(int32_t, int32_t) override
  {
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
  void leave() override
  {
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
  void updateKeyMap() override
  {
  }
  void updateKeyState() override
  {
  }

  bool blocked = false;
  uint32_t hotkeyID = 0;
  unsigned requests = 0;
  std::vector<std::pair<uint64_t, bool>> confirmations;
  std::vector<uint64_t> resumes;

protected:
  void handleSystemEvent(const Event &) override
  {
  }
  void updateButtons() override
  {
  }
  IKeyState *getKeyState() const override
  {
    return const_cast<NiceMock<MockKeyState> *>(&keys);
  }

private:
  NiceMock<MockKeyState> keys;
};

class TestAppUtil : public AppUtil
{
public:
  int run() override
  {
    return 0;
  }
  void startNode() override
  {
  }
  std::vector<std::string> getKeyboardLayoutList() override
  {
    return {"en"};
  }
  std::string getCurrentLanguageCode() override
  {
    return "en";
  }
};

class MemoryStream : public deskflow::IStream
{
public:
  void close() override
  {
  }
  uint32_t read(void *buffer, uint32_t size) override
  {
    const auto count = std::min<size_t>(size, input.size());
    std::memcpy(buffer, input.data(), count);
    input.erase(0, count);
    return count;
  }
  void feed(const std::string &payload)
  {
    input += payload;
  }
  void write(const void *buffer, uint32_t size) override
  {
    const std::string packet(static_cast<const char *>(buffer), size);
    if (!failOn.empty() && packet.starts_with(failOn)) {
      throw std::runtime_error("injected stream write failure");
    }
    packets.push_back(packet);
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
    return const_cast<MemoryStream *>(this);
  }
  bool isReady() const override
  {
    return false;
  }
  uint32_t getSize() const override
  {
    return input.size();
  }

  std::vector<std::string> codes() const
  {
    std::vector<std::string> result;
    for (const auto &packet : packets) {
      result.push_back(packet.substr(0, 4));
    }
    return result;
  }

  std::string input;
  std::string failOn;
  std::vector<std::string> packets;
};

class TestRemote : public ClientProxy1_9
{
public:
  TestRemote(const std::string &name, MemoryStream *stream, Server *server, IEventQueue *events)
      : ClientProxy1_9(name, stream, server, events)
  {
  }
  void getShape(int32_t &x, int32_t &y, int32_t &w, int32_t &h) const override
  {
    x = y = 0;
    w = 1200;
    h = 800;
  }
  void getCursorPos(int32_t &x, int32_t &y) const override
  {
    x = 600;
    y = 400;
  }
  bool supportsInputRelease() const override
  {
    return supportsBarrier;
  }
  void echo()
  {
    ASSERT_TRUE(parseMessage(reinterpret_cast<const uint8_t *>("CALV")));
  }
  void released(uint64_t generation = 7, bool success = true)
  {
    MemoryStream reply;
    ProtocolUtil::writef(&reply, kMsgDInputReleased, uint32_t(generation >> 32), uint32_t(generation), success ? 1 : 0);
    static_cast<MemoryStream *>(getStream())->feed(reply.packets.back().substr(4));
    ASSERT_TRUE(parseMessage(reinterpret_cast<const uint8_t *>("DRST")));
  }
  bool receiveRelease(const std::string &payload)
  {
    static_cast<MemoryStream *>(getStream())->feed(payload);
    return parseMessage(reinterpret_cast<const uint8_t *>("DRST"));
  }
  using ClientProxy1_3::keepAlive;
  bool supportsBarrier = true;
};

class CountAction : public InputFilter::Action
{
public:
  explicit CountAction(unsigned &count) : count(count)
  {
  }
  Action *clone() const override
  {
    return new CountAction(count);
  }
  std::string format() const override
  {
    return "count";
  }
  void perform(const Event &) override
  {
    ++count;
  }

private:
  unsigned &count;
};

class SharedInputServerTests : public testing::Test
{
protected:
  void SetUp() override
  {
    platform = new TestPlatform(queue);
    screen = std::make_unique<deskflow::Screen>(platform, &queue);
    primary = std::make_unique<PrimaryClient>("mac", screen.get());
    config.addScreen("mac");
    config.addScreen("win");
    config.addScreen("other");
    config.addOption("", kOptionClipboardSharing, 0);
    config.connect("mac", Direction::Left, 0, 1, "win", 0, 1);
    InputFilter::Rule rule(new InputFilter::KeystrokeCondition(&queue, 'x', KeyModifierControl));
    rule.adoptAction(new CountAction(actions), true);
    config.getInputFilter()->addFilterRule(rule);
    server = std::make_unique<Server>(config, primary.get(), screen.get(), &queue);
    stream = new MemoryStream;
    remote = new TestRemote("win", stream, server.get(), &queue);
    server->adoptClient(remote);
    queue.drain();
    stream->packets.clear();
  }

  void TearDown() override
  {
    if (stream) {
      stream->failOn.clear();
    }
    server.reset();
    primary.reset();
    screen.reset();
  }

  void switchTo(const std::string &name)
  {
    Server::SwitchToScreenInfo info(name);
    queue.dispatchEvent(Event(
        EventTypes::ServerSwitchToScreen, config.getInputFilter(), static_cast<void *>(&info),
        Event::EventFlags::DontFreeData
    ));
  }

  void key(EventTypes type = EventTypes::KeyStateKeyDown, const std::string &destinations = "")
  {
    auto *info = IKeyState::KeyInfo::alloc('a', KeyModifierControl, 9, 1);
    info->m_screens = destinations;
    queue.addEvent(Event(type, platform, info));
  }

  void prepare(uint64_t generation = 7)
  {
    platform->blocked = true;
    queue.addEvent(Event(EventTypes::SharedInputLockPrepare, platform, new deskflow::SharedInputLockEvent(generation)));
    queue.drain();
  }

  TestAppUtil appUtil;
  TestQueue queue;
  deskflow::server::Config config{&queue};
  TestPlatform *platform;
  std::unique_ptr<deskflow::Screen> screen;
  std::unique_ptr<PrimaryClient> primary;
  std::unique_ptr<Server> server;
  TestRemote *remote;
  MemoryStream *stream;
  unsigned actions = 0;
};

TEST_F(SharedInputServerTests, RemoteActiveReleasesBeforeAcknowledgingAndDropsQueuedInput)
{
  switchTo("win");
  key();
  queue.addEvent(Event(EventTypes::PrimaryScreenButtonDown, platform, IPrimaryScreen::ButtonInfo::alloc(kButtonLeft, 0))
  );
  queue.drain();
  remote->keepAlive(); // old echo must not satisfy the release barrier
  stream->packets.clear();

  key();
  key(EventTypes::KeyStateKeyRepeat);
  key(EventTypes::KeyStateKeyUp);
  queue.addEvent(Event(EventTypes::PrimaryScreenHotkeyDown, platform, IPrimaryScreen::HotKeyInfo::alloc(1)));
  queue.addEvent(Event(EventTypes::PrimaryScreenMotionOnSecondary, platform, IPrimaryScreen::MotionInfo::alloc(10, 10))
  );
  queue.addEvent(Event(EventTypes::PrimaryScreenWheel, platform, IPrimaryScreen::WheelInfo::alloc(0, 120)));
  prepare();
  EXPECT_EQ(stream->codes(), (std::vector<std::string>{"CRST"}));
  ASSERT_EQ(stream->packets.front().size(), 16);
  EXPECT_EQ(stream->packets.front()[15], 1u << kButtonLeft);
  EXPECT_EQ(actions, 0);
  EXPECT_TRUE(platform->confirmations.empty());
  remote->echo();
  queue.drain();
  EXPECT_TRUE(platform->confirmations.empty());
  remote->released();
  queue.drain();
  EXPECT_EQ(platform->confirmations, (std::vector<std::pair<uint64_t, bool>>{{7, true}}));
}

TEST_F(SharedInputServerTests, MacActiveStillReleasesBroadcastInputAndWaitsForEveryClient)
{
  auto *secondStream = new MemoryStream;
  auto *second = new TestRemote("other", secondStream, server.get(), &queue);
  server->adoptClient(second);
  Server::KeyboardBroadcastInfo broadcast(Server::KeyboardBroadcastInfo::kOn, "*");
  queue.dispatchEvent(Event(
      EventTypes::ServerKeyboardBroadcast, config.getInputFilter(), static_cast<void *>(&broadcast),
      Event::EventFlags::DontFreeData
  ));
  key(EventTypes::KeyStateKeyDown, "*");
  queue.drain();
  stream->packets.clear();
  secondStream->packets.clear();
  prepare();
  EXPECT_EQ(stream->codes(), (std::vector<std::string>{"CRST"}));
  EXPECT_EQ(secondStream->codes(), (std::vector<std::string>{"CRST"}));
  remote->released();
  queue.drain();
  EXPECT_TRUE(platform->confirmations.empty());
  second->released();
  queue.drain();
  EXPECT_EQ(platform->confirmations, (std::vector<std::pair<uint64_t, bool>>{{7, true}}));
}

TEST_F(SharedInputServerTests, ResumeDrainsOldInputAndKeepsOriginalDestination)
{
  switchTo("win");
  prepare();
  remote->released();
  queue.drain();
  stream->packets.clear();
  key();           // queued before the resume boundary
  switchTo("mac"); // a queued action must also be blocked
  queue.addEvent(Event(EventTypes::SharedInputLockResume, platform, new deskflow::SharedInputLockEvent(6)));
  queue.drain();
  EXPECT_TRUE(platform->resumes.empty());
  EXPECT_TRUE(stream->packets.empty());
  queue.addEvent(Event(EventTypes::SharedInputLockResume, platform, new deskflow::SharedInputLockEvent(7)));
  queue.drain();
  EXPECT_EQ(platform->resumes, (std::vector<uint64_t>{7}));
  key();
  queue.drain();
  EXPECT_EQ(stream->codes(), (std::vector<std::string>{"DKDL"}));
}

TEST_F(SharedInputServerTests, WriteFailureCannotConfirmLocked)
{
  switchTo("win");
  stream->failOn = "CRST";
  prepare();
  EXPECT_EQ(platform->confirmations, (std::vector<std::pair<uint64_t, bool>>{{7, false}}));
}

TEST_F(SharedInputServerTests, UnsupportedProtocolDoesNotPretendToRelease)
{
  switchTo("win");
  remote->supportsBarrier = false;
  stream->packets.clear();
  prepare();
  EXPECT_TRUE(stream->packets.empty());
  EXPECT_EQ(platform->confirmations, (std::vector<std::pair<uint64_t, bool>>{{7, false}}));
}

TEST_F(SharedInputServerTests, EmptyLocalSessionRequiresNoRemoteReset)
{
  prepare();
  EXPECT_TRUE(stream->packets.empty());
  EXPECT_EQ(platform->confirmations, (std::vector<std::pair<uint64_t, bool>>{{7, true}}));
}

TEST_F(SharedInputServerTests, DisconnectCancelsPendingReceiptAndRemovesItsHandler)
{
  switchTo("win");
  prepare();
  const auto target = remote->getEventTarget();
  queue.addEvent(Event(EventTypes::ClientProxyDisconnected, remote));
  queue.drain();
  remote = nullptr;
  stream = nullptr;
  EXPECT_EQ(platform->confirmations, (std::vector<std::pair<uint64_t, bool>>{{7, false}}));
  queue.addEvent(Event(EventTypes::SharedInputLockRemoteReady, target, new deskflow::SharedInputLockEvent(7)));
  queue.drain();
  EXPECT_EQ(platform->confirmations.size(), 1);
}

TEST_F(SharedInputServerTests, FailedReleaseCannotLaterSucceedFromAnEarlierClientsEcho)
{
  auto *secondStream = new MemoryStream;
  auto *second = new TestRemote("other", secondStream, server.get(), &queue);
  server->adoptClient(second);
  key(EventTypes::KeyStateKeyDown, "*");
  queue.drain();
  secondStream->failOn = "CRST";
  prepare();
  remote->released();
  queue.drain();
  EXPECT_EQ(platform->confirmations, (std::vector<std::pair<uint64_t, bool>>{{7, false}}));
  secondStream->failOn.clear();
}

TEST_F(SharedInputServerTests, UnrelatedDisconnectDoesNotCancelConfirmedLock)
{
  auto *second = new TestRemote("other", new MemoryStream, server.get(), &queue);
  server->adoptClient(second);
  queue.drain();
  switchTo("win");
  key();
  queue.drain();
  prepare();
  remote->released();
  queue.drain();
  queue.addEvent(Event(EventTypes::ClientProxyDisconnected, second));
  queue.drain();
  EXPECT_EQ(platform->confirmations, (std::vector<std::pair<uint64_t, bool>>{{7, true}}));
}

TEST_F(SharedInputServerTests, PendingDisconnectCannotBeOverruledByAnotherClientsResult)
{
  auto *second = new TestRemote("other", new MemoryStream, server.get(), &queue);
  server->adoptClient(second);
  key(EventTypes::KeyStateKeyDown, "*");
  queue.drain();
  prepare();
  queue.addEvent(Event(EventTypes::ClientProxyDisconnected, second));
  queue.drain();
  remote->released();
  queue.drain();
  EXPECT_EQ(platform->confirmations, (std::vector<std::pair<uint64_t, bool>>{{7, false}}));
}

TEST_F(SharedInputServerTests, FailedClientResultIsNotReplacedByADuplicateSuccess)
{
  switchTo("win");
  prepare();
  remote->released(7, false);
  remote->released(7, true);
  queue.drain();
  EXPECT_EQ(platform->confirmations, (std::vector<std::pair<uint64_t, bool>>{{7, false}}));
}

TEST_F(SharedInputServerTests, ReleaseResultMustMatchTheFullGeneration)
{
  switchTo("win");
  constexpr uint64_t generation = (uint64_t(3) << 32) | 7;
  prepare(generation);
  remote->released(7);
  remote->echo();
  queue.drain();
  EXPECT_TRUE(platform->confirmations.empty());
  remote->released(generation);
  queue.drain();
  EXPECT_EQ(platform->confirmations, (std::vector<std::pair<uint64_t, bool>>{{generation, true}}));
}

TEST_F(SharedInputServerTests, LegacyClientRejectsLockBeforeClosingTheCaptureGate)
{
  switchTo("win");
  remote->supportsBarrier = false;
  queue.addEvent(Event(EventTypes::ServerLockInput, config.getInputFilter(), new deskflow::SharedInputLockRequest(30)));
  queue.drain();
  EXPECT_EQ(platform->requests, 0);
  EXPECT_FALSE(platform->blocked);
  stream->packets.clear();
  key();
  queue.drain();
  EXPECT_EQ(stream->codes(), (std::vector<std::string>{"DKDL"}));
}

TEST_F(SharedInputServerTests, MalformedReleaseResultCannotConfirmLocked)
{
  switchTo("win");
  prepare();
  MemoryStream invalid;
  ProtocolUtil::writef(&invalid, kMsgDInputReleased, 0, 7, 2);
  EXPECT_FALSE(remote->receiveRelease(invalid.packets.back().substr(4)));
  queue.drain();
  EXPECT_TRUE(platform->confirmations.empty());
}

class ReleasePlatform : public TestPlatform
{
public:
  using TestPlatform::TestPlatform;
  bool supportsInputRelease() const override
  {
    return releaseSupported;
  }
  bool isPrimary() const override
  {
    return false;
  }
  bool canLeave() override
  {
    ++leaveChecks;
    return false;
  }
  void fakeKeyDown(KeyID, KeyModifierMask, KeyButton, const std::string &) override
  {
    held = true;
  }
  void fakeAllKeysUp() override
  {
    held = false;
    ++releases;
  }
  void fakeMouseMove(int32_t, int32_t) override
  {
    ++moves;
  }
  bool releaseInput(uint32_t buttons) override
  {
    releasedButtons = buttons;
    fakeAllKeysUp();
    return injectionSucceeded;
  }
  bool held = false, injectionSucceeded = true, releaseSupported = true;
  unsigned leaveChecks = 0, releases = 0, moves = 0;
  uint32_t releasedButtons = 0;
};

class NoSockets : public ISocketFactory
{
public:
  IDataSocket *create(IArchNetwork::AddressFamily, SecurityLevel) const override
  {
    ADD_FAILURE() << "unexpected socket creation";
    return nullptr;
  }
  IListenSocket *createListen(IArchNetwork::AddressFamily, SecurityLevel) const override
  {
    ADD_FAILURE() << "unexpected listener creation";
    return nullptr;
  }
};

class ReceiveProxy : public ServerProxy
{
public:
  using ServerProxy::ServerProxy;
  bool receive(const char *code)
  {
    return parseMessage(reinterpret_cast<const uint8_t *>(code)) == ConnectionResult::Okay;
  }
};

static void checkClientRelease(bool success)
{
  TestAppUtil app;
  TestQueue queue;
  auto *platform = new ReleasePlatform(queue);
  platform->injectionSucceeded = success;
  deskflow::Screen screen(platform, &queue);
  Client client(&queue, "win", NetworkAddress(), new NoSockets, &screen);
  client.setOptions({kOptionClipboardSharing, 0});
  MemoryStream stream, request;
  ReceiveProxy proxy(&client, &stream, &queue);
  client.enter(100, 100, 1, 0, false);
  client.keyDown('a', 0, 9, "en");
  ASSERT_TRUE(platform->held);
  const auto moves = platform->moves;
  ProtocolUtil::writef(&request, kMsgCInputRelease, 3, 7, 1u << kButtonLeft);
  stream.feed(request.packets.back().substr(4));
  ASSERT_TRUE(proxy.receive("CRST"));
  ASSERT_EQ(stream.packets.front().size(), 13);
  EXPECT_EQ(stream.codes().front(), "DRST");
  EXPECT_EQ(stream.packets.front()[7], 3);
  EXPECT_EQ(stream.packets.front()[11], 7);
  EXPECT_EQ(stream.packets.front()[12], success ? 1 : 0);
  EXPECT_EQ(platform->releases, 1);
  EXPECT_FALSE(platform->held);
  EXPECT_EQ(platform->releasedButtons, 1u << kButtonLeft);
  EXPECT_EQ(platform->leaveChecks, 0);
  EXPECT_EQ(platform->moves, moves);
}

TEST(SharedInputClientTests, CheckedResetBypassesRefusedLeaveAndPreservesCursor)
{
  checkClientRelease(true);
}
TEST(SharedInputClientTests, InjectionFailureIsReportedAsFailure)
{
  checkClientRelease(false);
}

TEST(SharedInputClientTests, ReleaseCapabilityComesFromTheSecondaryPlatform)
{
  TestAppUtil app;
  TestQueue queue;
  auto *platform = new ReleasePlatform(queue);
  deskflow::Screen screen(platform, &queue);
  EXPECT_TRUE(screen.supportsInputRelease());
  platform->releaseSupported = false;
  EXPECT_FALSE(screen.supportsInputRelease());
  deskflow::Screen primary(new TestPlatform(queue), &queue);
  EXPECT_FALSE(primary.supportsInputRelease());
}

TEST(SharedInputClientTests, InvalidResetDoesNotReleaseOrAcknowledge)
{
  TestAppUtil app;
  TestQueue queue;
  auto *platform = new ReleasePlatform(queue);
  deskflow::Screen screen(platform, &queue);
  Client client(&queue, "win", NetworkAddress(), new NoSockets, &screen);
  client.setOptions({kOptionClipboardSharing, 0});
  MemoryStream zeroGeneration, unknownButton;
  ProtocolUtil::writef(&zeroGeneration, kMsgCInputRelease, 0, 0, 0);
  ProtocolUtil::writef(&unknownButton, kMsgCInputRelease, 0, 7, 1u << NumButtonIDs);
  for (const auto &payload :
       {zeroGeneration.packets.back().substr(4), unknownButton.packets.back().substr(4), std::string(5, '\0')}) {
    MemoryStream stream;
    ReceiveProxy proxy(&client, &stream, &queue);
    stream.feed(payload);
    EXPECT_FALSE(proxy.receive("CRST"));
    EXPECT_TRUE(stream.packets.empty());
    EXPECT_EQ(platform->releases, 0);
  }
}

} // namespace
