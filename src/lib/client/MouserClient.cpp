/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/MouserClient.h"

#include "base/Log.h"

#include <QJsonDocument>
#include <QJsonObject>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <cerrno>
#include <chrono>
#include <cstring>

namespace {

const size_t kMaxQueuedLines = 256;

void platformCloseSocket(int fd)
{
#if defined(_WIN32)
  ::closesocket(fd);
#else
  ::close(fd);
#endif
}

bool sendAll(int fd, const std::string &payload)
{
  size_t sent = 0;
  while (sent < payload.size()) {
    const auto wrote = ::send(fd, payload.data() + sent, payload.size() - sent, 0);
    if (wrote <= 0) {
      return false;
    }
    sent += static_cast<size_t>(wrote);
  }
  return true;
}

// Blocking read of a single reply line (used only for the hello reply).
bool readReplyLine(int fd, std::string &line)
{
  line.clear();
  char ch = 0;
  while (line.size() < 64 * 1024) {
    const auto received = ::recv(fd, &ch, 1, 0);
    if (received <= 0) {
      return false;
    }
    if (ch == '\n') {
      return true;
    }
    line.push_back(ch);
  }
  return false;
}

} // namespace

MouserClient::MouserClient(int port, std::string token) : m_port(port), m_token(std::move(token))
{
  m_thread = std::thread([this] { workerLoop(); });
}

MouserClient::~MouserClient()
{
  m_running = false;
  m_wake.notify_all();
  // The worker owns m_fd; join first (its blocking ops are bounded by the
  // handshake receive timeout), then tear the socket down.
  if (m_thread.joinable()) {
    m_thread.join();
  }
  disconnect();
}

void MouserClient::deliver(const std::string &line)
{
  {
    std::scoped_lock lock{m_mutex};
    if (m_queue.size() >= kMaxQueuedLines) {
      m_queue.pop_front(); // drop oldest; gestures are ephemeral
    }
    m_queue.push_back(line);
  }
  m_wake.notify_one();
}

void MouserClient::workerLoop()
{
  while (m_running) {
    std::string line;
    {
      std::unique_lock lock{m_mutex};
      m_wake.wait(lock, [this] { return !m_running || !m_queue.empty(); });
      if (!m_running) {
        return;
      }
      line = m_queue.front();
      m_queue.pop_front();
    }

    if (!ensureConnected()) {
      // Mouser is not reachable; drop the line and back off briefly so a
      // burst of events does not spin the CPU on failed connects.
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      continue;
    }
    drainReplies();
    if (!sendLine(line)) {
      disconnect();
    }
  }
}

bool MouserClient::ensureConnected()
{
  if (m_fd >= 0) {
    return true;
  }
  if (m_token.empty()) {
    return false;
  }

  const int fd = static_cast<int>(::socket(AF_INET, SOCK_STREAM, 0));
  if (fd < 0) {
    return false;
  }
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(m_port));
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  // Bound the connect (loopback, but a wedged peer's full accept queue
  // can still stall a blocking connect) so the worker never hangs.
#if !defined(_WIN32)
  const int connectFlags = ::fcntl(fd, F_GETFL, 0);
  ::fcntl(fd, F_SETFL, connectFlags | O_NONBLOCK);
#else
  u_long nonBlockingConnect = 1;
  ::ioctlsocket(fd, FIONBIO, &nonBlockingConnect);
#endif
  const auto rc = ::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
  bool connected = (rc == 0);
  if (!connected) {
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(fd, &writeSet);
    timeval timeout{};
    timeout.tv_sec = 2;
    if (::select(fd + 1, nullptr, &writeSet, nullptr, &timeout) == 1) {
      int error = 0;
#if defined(_WIN32)
      int errorLen = sizeof(error);
#else
      socklen_t errorLen = sizeof(error);
#endif
      ::getsockopt(fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&error), &errorLen);
      connected = (error == 0);
    }
  }
  if (!connected) {
    platformCloseSocket(fd);
    return false;
  }
#if !defined(_WIN32)
  ::fcntl(fd, F_SETFL, connectFlags); // blocking again for the hello round-trip
#else
  u_long blockingAgain = 0;
  ::ioctlsocket(fd, FIONBIO, &blockingAgain);
#endif

  // Bound the hello round-trip so a wedged peer cannot hang the worker.
#if !defined(_WIN32)
  timeval timeout{};
  timeout.tv_sec = 3;
  ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout));
#else
  DWORD timeoutMs = 3000;
  ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeoutMs), sizeof(timeoutMs));
#endif

  QJsonObject hello;
  hello[QStringLiteral("type")] = QStringLiteral("hello");
  hello[QStringLiteral("token")] = QString::fromStdString(m_token);
  hello[QStringLiteral("version")] = 1;
  std::string helloLine = QJsonDocument(hello).toJson(QJsonDocument::Compact).toStdString();
  helloLine.push_back('\n');

  std::string reply;
  if (!sendAll(fd, helloLine) || !readReplyLine(fd, reply)) {
    platformCloseSocket(fd);
    return false;
  }
  const auto doc = QJsonDocument::fromJson(QByteArray(reply.data(), static_cast<int>(reply.size())));
  if (!doc.isObject() || !doc.object()[QStringLiteral("ok")].toBool()) {
    LOG_WARN("mouser client: local mouser rejected hello: %.128s", reply.c_str());
    platformCloseSocket(fd);
    return false;
  }

#if !defined(_WIN32)
  // Replies after the hello are advisory; drain them non-blockingly so the
  // peer's send buffer can never fill up.
  const int flags = ::fcntl(fd, F_GETFL, 0);
  ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
  u_long nonblocking = 1;
  ::ioctlsocket(fd, FIONBIO, &nonblocking);
#endif

  m_fd = fd;
  LOG_INFO("mouser client: connected to local mouser on 127.0.0.1:%d", m_port);
  return true;
}

bool MouserClient::sendLine(const std::string &line)
{
  std::string payload = line;
  payload.push_back('\n');
  size_t sent = 0;
  while (sent < payload.size()) {
    const auto wrote = ::send(m_fd, payload.data() + sent, payload.size() - sent, 0);
    if (wrote > 0) {
      sent += static_cast<size_t>(wrote);
      continue;
    }
#if !defined(_WIN32)
    if (wrote < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
#else
    if (wrote < 0 && WSAGetLastError() == WSAEWOULDBLOCK) {
#endif
      if (!m_running) {
        return false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }
    return false;
  }
  return true;
}

void MouserClient::drainReplies()
{
  char buffer[4096];
  while (true) {
    const auto received = ::recv(m_fd, buffer, sizeof(buffer), 0);
    if (received > 0) {
      continue; // discard acks
    }
    if (received == 0) {
      disconnect(); // peer closed
      return;
    }
#if !defined(_WIN32)
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return;
    }
#else
    if (WSAGetLastError() == WSAEWOULDBLOCK) {
      return;
    }
#endif
    disconnect();
    return;
  }
}

void MouserClient::disconnect()
{
  if (m_fd >= 0) {
    platformCloseSocket(m_fd);
    m_fd = -1;
  }
}
