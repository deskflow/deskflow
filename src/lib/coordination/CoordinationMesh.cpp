/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "coordination/CoordinationMesh.h"

#include "base/Log.h"

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketLen = int;
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketLen = socklen_t;
#endif

#include <cerrno>
#include <chrono>
#include <cstring>
#include <utility>

namespace deskflow::coordination {

namespace {

const int kSendConnectTimeoutMs = 700;
const int kClientReadTimeoutMs = 2000;
const size_t kMaxLineBytes = 16 * 1024;
const int kMaxConcurrentClients = 8;

void platformCloseSocket(int fd)
{
#if defined(_WIN32)
  ::closesocket(fd);
#else
  ::close(fd);
#endif
}

void setReceiveTimeout(int fd, int timeoutMs)
{
#if defined(_WIN32)
  DWORD value = static_cast<DWORD>(timeoutMs);
  ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&value), sizeof(value));
#else
  timeval value{};
  value.tv_sec = timeoutMs / 1000;
  value.tv_usec = (timeoutMs % 1000) * 1000;
  ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&value), sizeof(value));
#endif
}

//! Resolve + connect with a bounded timeout; returns fd or -1.
int connectWithTimeout(const std::string &host, int port, int timeoutMs)
{
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo *results = nullptr;
  const std::string service = std::to_string(port);
  if (::getaddrinfo(host.c_str(), service.c_str(), &hints, &results) != 0 || results == nullptr) {
    return -1;
  }

  int fd = -1;
  for (addrinfo *entry = results; entry != nullptr; entry = entry->ai_next) {
    fd = static_cast<int>(::socket(entry->ai_family, entry->ai_socktype, entry->ai_protocol));
    if (fd < 0) {
      continue;
    }

#if defined(_WIN32)
    u_long nonBlocking = 1;
    ::ioctlsocket(fd, FIONBIO, &nonBlocking);
#else
    const int flags = ::fcntl(fd, F_GETFL, 0);
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif

    const auto rc = ::connect(fd, entry->ai_addr, static_cast<SocketLen>(entry->ai_addrlen));
    bool connected = (rc == 0);
    if (!connected) {
      fd_set writeSet;
      FD_ZERO(&writeSet);
      FD_SET(fd, &writeSet);
      timeval timeout{};
      timeout.tv_sec = timeoutMs / 1000;
      timeout.tv_usec = (timeoutMs % 1000) * 1000;
      if (::select(fd + 1, nullptr, &writeSet, nullptr, &timeout) == 1) {
        int error = 0;
        SocketLen errorLen = sizeof(error);
        ::getsockopt(fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&error), &errorLen);
        connected = (error == 0);
      }
    }

    if (connected) {
#if defined(_WIN32)
      u_long blocking = 0;
      ::ioctlsocket(fd, FIONBIO, &blocking);
#else
      const int restored = ::fcntl(fd, F_GETFL, 0);
      ::fcntl(fd, F_SETFL, restored & ~O_NONBLOCK);
#endif
      break;
    }
    platformCloseSocket(fd);
    fd = -1;
  }

  ::freeaddrinfo(results);
  return fd;
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

} // namespace

CoordinationMesh::CoordinationMesh(int port, std::string token, Receiver receiver)
    : m_port(port),
      m_token(std::move(token)),
      m_receiver(std::move(receiver))
{
  // do nothing
}

CoordinationMesh::~CoordinationMesh()
{
  stop();
}

bool CoordinationMesh::start()
{
  m_listenFd = static_cast<int>(::socket(AF_INET, SOCK_STREAM, 0));
  if (m_listenFd < 0) {
    LOG_WARN("coordination: mesh socket() failed: %s", std::strerror(errno));
    return false;
  }
  const int reuse = 1;
  ::setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&reuse), sizeof(reuse));

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(static_cast<uint16_t>(m_port));
  address.sin_addr.s_addr = htonl(INADDR_ANY); // peers are remote by definition

  if (::bind(m_listenFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0) {
    LOG_WARN("coordination: mesh failed to bind port %d: %s", m_port, std::strerror(errno));
    platformCloseSocket(m_listenFd);
    m_listenFd = -1;
    return false;
  }
  if (::listen(m_listenFd, 4) != 0) {
    LOG_WARN("coordination: mesh listen() failed: %s", std::strerror(errno));
    platformCloseSocket(m_listenFd);
    m_listenFd = -1;
    return false;
  }

  m_running = true;
  m_thread = std::thread([this] { serveLoop(); });
  LOG_INFO("coordination: mesh listening on port %d", m_port);
  return true;
}

void CoordinationMesh::stop()
{
  m_running = false;
  if (m_listenFd >= 0) {
    platformCloseSocket(m_listenFd);
    m_listenFd = -1;
  }
  if (m_thread.joinable()) {
    m_thread.join();
  }
  // Unblock every in-flight handler, then wait for them to drain so a
  // handler thread can never touch *this after destruction.
  {
    std::scoped_lock lock{m_clientsMutex};
    for (const int fd : m_clientFds) {
#if defined(_WIN32)
      ::shutdown(fd, SD_BOTH);
#else
      ::shutdown(fd, SHUT_RDWR);
#endif
    }
  }
  std::unique_lock lock{m_clientsMutex};
  m_clientsDone.wait_for(lock, std::chrono::seconds(5), [this] { return m_activeClients.load() == 0; });
}

void CoordinationMesh::sendTo(const std::string &host, const std::string &line)
{
  const int fd = connectWithTimeout(host, m_port, kSendConnectTimeoutMs);
  if (fd < 0) {
    return; // unreachable peers are normal (asleep / off)
  }
  std::string payload = line;
  payload.push_back('\n');
  sendAll(fd, payload);
  platformCloseSocket(fd);
}

bool CoordinationMesh::probe(const std::string &host, int timeoutMs)
{
  const int fd = connectWithTimeout(host, m_port, timeoutMs);
  if (fd < 0) {
    return false;
  }
  platformCloseSocket(fd);
  return true;
}

bool CoordinationMesh::probeDeskflowPort(int port, int timeoutMs)
{
  const int fd = connectWithTimeout("127.0.0.1", port, timeoutMs);
  if (fd < 0) {
    return false;
  }
  platformCloseSocket(fd);
  return true;
}

std::string CoordinationMesh::query(const std::string &host, const std::string &line)
{
  const int fd = connectWithTimeout(host, m_port, kSendConnectTimeoutMs);
  if (fd < 0) {
    return {};
  }
  std::string payload = line;
  payload.push_back('\n');
  std::string reply;
  if (sendAll(fd, payload)) {
    setReceiveTimeout(fd, 1000);
    char buffer[4096];
    while (reply.find('\n') == std::string::npos && reply.size() < kMaxLineBytes) {
      const auto received = ::recv(fd, buffer, sizeof(buffer), 0);
      if (received <= 0) {
        break;
      }
      reply.append(buffer, static_cast<size_t>(received));
    }
  }
  platformCloseSocket(fd);
  const auto newline = reply.find('\n');
  return newline == std::string::npos ? reply : reply.substr(0, newline);
}

bool CoordinationMesh::tokenOk(const Message &message) const
{
  if (m_token.empty()) {
    return true;
  }
  if (message.token.size() != m_token.size()) {
    return false;
  }
  // Constant-time compare; the mesh is reachable from the LAN.
  unsigned char diff = 0;
  for (size_t i = 0; i < m_token.size(); ++i) {
    diff |= static_cast<unsigned char>(m_token[i] ^ message.token[i]);
  }
  return diff == 0;
}

void CoordinationMesh::serveLoop()
{
  while (m_running) {
    sockaddr_in peer{};
    SocketLen peerLen = sizeof(peer);
    const int clientFd = static_cast<int>(::accept(m_listenFd, reinterpret_cast<sockaddr *>(&peer), &peerLen));
    if (clientFd < 0) {
      break; // listener closed by stop()
    }
    if (!m_running) {
      platformCloseSocket(clientFd);
      break;
    }
    if (m_activeClients.load() >= kMaxConcurrentClients) {
      // A stalled or hostile peer set must not starve the mesh; excess
      // connections are refused rather than queued behind them.
      platformCloseSocket(clientFd);
      continue;
    }
    {
      std::scoped_lock lock{m_clientsMutex};
      m_clientFds.insert(clientFd);
    }
    ++m_activeClients;
    std::thread([this, clientFd] {
      handleClient(clientFd);
      {
        std::scoped_lock lock{m_clientsMutex};
        m_clientFds.erase(clientFd);
      }
      platformCloseSocket(clientFd);
      --m_activeClients;
      m_clientsDone.notify_all();
    }).detach();
  }
}

void CoordinationMesh::handleClient(int clientFd)
{
  setReceiveTimeout(clientFd, kClientReadTimeoutMs);

  std::string carry;
  char buffer[4096];
  while (m_running) {
    const auto newline = carry.find('\n');
    if (newline != std::string::npos) {
      const std::string line = carry.substr(0, newline);
      carry.erase(0, newline + 1);
      if (line.empty()) {
        continue;
      }
      const Message message = protocol::decode(line);
      if (message.type == Message::Type::Invalid) {
        continue;
      }
      if (!tokenOk(message)) {
        LOG_DEBUG("coordination: dropping message with bad token");
        continue;
      }
      m_receiver(message, [clientFd](const std::string &reply) {
        std::string payload = reply;
        payload.push_back('\n');
        sendAll(clientFd, payload);
      });
      continue;
    }
    if (carry.size() > kMaxLineBytes) {
      return;
    }
    const auto received = ::recv(clientFd, buffer, sizeof(buffer), 0);
    if (received <= 0) {
      return; // EOF or timeout: legacy senders are one-shot
    }
    carry.append(buffer, static_cast<size_t>(received));
  }
}

} // namespace deskflow::coordination
