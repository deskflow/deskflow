/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "TCPListenSocketTests.h"

#include "base/EventQueue.h"
#include "net/IDataSocket.h"
#include "net/NetworkAddress.h"
#include "net/SocketMultiplexer.h"
#include "net/TCPListenSocket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <QScopeGuard>

#include <atomic>
#include <chrono>
#include <thread>

namespace {
constexpr int kPort = 47213;

int connectToListener()
{
  const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(kPort);
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  ::connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address));
  return fd;
}

// true once the listener has announced `count` connections in total
bool waitForAnnounced(const std::atomic<int> &announced, int count)
{
  for (int i = 0; i < 40 && announced < count; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  return announced >= count;
}
} // namespace

void TCPListenSocketTests::initTestCase()
{
  m_arch.init();
}

void TCPListenSocketTests::acceptSurvivesPeerReset()
{
  EventQueue events;
  SocketMultiplexer multiplexer;
  TCPListenSocket listener(&events, &multiplexer, IArchNetwork::AddressFamily::INet);
  NetworkAddress address("127.0.0.1", kPort);
  address.resolve();
  listener.bind(address);

  std::atomic<int> announced{0};
  events.addHandler(EventTypes::ListenSocketConnecting, &listener, [&announced](const Event &) { ++announced; });
  std::thread loop([&events] { events.loop(); });
  const auto stopLoop = qScopeGuard([&events, &loop] {
    events.addEvent(Event(EventTypes::Quit));
    loop.join();
  });

  // a peer that resets before accept(): macOS then fails setsockopt(TCP_NODELAY) with EINVAL (#10068)
  const int peer = connectToListener();
  const linger reset{1, 0};
  ::setsockopt(peer, SOL_SOCKET, SO_LINGER, &reset, sizeof(reset));
  ::close(peer);
  QVERIFY(waitForAnnounced(announced, 1));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  QVERIFY_THROWS_NO_EXCEPTION(std::ignore = listener.accept());

  // the listener must keep accepting: the next client is still announced
  const int next = connectToListener();
  const bool stillListening = waitForAnnounced(announced, 2);
  ::close(next);
  QVERIFY(stillListening);
}

QTEST_MAIN(TCPListenSocketTests)
