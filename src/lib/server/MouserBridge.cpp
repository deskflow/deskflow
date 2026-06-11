/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/MouserBridge.h"

#include "base/IEventQueue.h"
#include "base/Log.h"

#include <QJsonDocument>
#include <QJsonObject>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketLen = int;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketLen = socklen_t;
#endif

#include <cerrno>
#include <cstring>

namespace {

const size_t kMaxLineBytes = 64 * 1024;

void platformCloseSocket(int fd)
{
#if defined(_WIN32)
  ::closesocket(fd);
#else
  ::close(fd);
#endif
}

// Constant-time-ish comparison so the token check does not leak length
// prefixes through timing. Loopback-only, but cheap to do right.
bool tokenMatches(const std::string &expected, const std::string &provided)
{
  if (expected.empty()) {
    return false;
  }
  unsigned char diff = expected.size() == provided.size() ? 0 : 1;
  const size_t n = expected.size();
  for (size_t i = 0; i < n; ++i) {
    const char p = i < provided.size() ? provided[i] : 0;
    diff |= static_cast<unsigned char>(expected[i] ^ p);
  }
  return diff == 0;
}

QJsonObject parseObject(const std::string &line)
{
  const auto doc = QJsonDocument::fromJson(QByteArray(line.data(), static_cast<int>(line.size())));
  return doc.isObject() ? doc.object() : QJsonObject();
}

std::string serializeObject(const QJsonObject &object)
{
  return QJsonDocument(object).toJson(QJsonDocument::Compact).toStdString();
}

} // namespace

MouserBridge::MouserBridge(IEventQueue *events, void *eventTarget, int port, std::string token)
    : m_events(events),
      m_eventTarget(eventTarget),
      m_port(port),
      m_token(std::move(token))
{
  // do nothing
}

MouserBridge::~MouserBridge()
{
  stop();
}

bool MouserBridge::start()
{
  if (m_token.empty()) {
    LOG_WARN("mouser bridge not started: no token configured");
    return false;
  }

  m_listenFd = static_cast<int>(::socket(AF_INET, SOCK_STREAM, 0));
  if (m_listenFd < 0) {
    LOG_WARN("mouser bridge socket() failed: %s", std::strerror(errno));
    return false;
  }

  const int reuse = 1;
  ::setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&reuse), sizeof(reuse));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(m_port));
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // loopback only, by design

  if (::bind(m_listenFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
    LOG_WARN("mouser bridge failed to bind 127.0.0.1:%d: %s", m_port, std::strerror(errno));
    closeFd(m_listenFd);
    return false;
  }
  if (::listen(m_listenFd, 1) != 0) {
    LOG_WARN("mouser bridge listen() failed: %s", std::strerror(errno));
    closeFd(m_listenFd);
    return false;
  }

  m_running = true;
  m_thread = std::thread([this] { serveLoop(); });
  LOG_INFO("mouser bridge listening on 127.0.0.1:%d", m_port);
  return true;
}

void MouserBridge::stop()
{
  m_running = false;
  {
    std::scoped_lock lock{m_clientMutex};
    if (m_clientFd >= 0) {
#if !defined(_WIN32)
      ::shutdown(m_clientFd, SHUT_RDWR);
#else
      ::shutdown(m_clientFd, SD_BOTH);
#endif
    }
  }
  closeFd(m_listenFd);
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void MouserBridge::notifyFocus(const std::string &screenName, bool isLocal)
{
  QJsonObject object;
  object[QStringLiteral("type")] = QStringLiteral("focus");
  object[QStringLiteral("screen")] = QString::fromStdString(screenName);
  object[QStringLiteral("local")] = isLocal;
  const std::string line = serializeObject(object);

  {
    std::scoped_lock lock{m_focusMutex};
    m_focusLine = line;
  }

  std::scoped_lock lock{m_clientMutex};
  if (m_clientFd >= 0) {
    sendLine(m_clientFd, line);
  }
}

void MouserBridge::serveLoop()
{
  while (m_running) {
    sockaddr_in peer{};
    SocketLen peerLen = sizeof(peer);
    const int clientFd = static_cast<int>(::accept(m_listenFd, reinterpret_cast<sockaddr *>(&peer), &peerLen));
    if (clientFd < 0) {
      if (m_running) {
        LOG_DEBUG("mouser bridge accept failed: %s", std::strerror(errno));
      }
      break; // listener closed by stop()
    }

    {
      std::scoped_lock lock{m_clientMutex};
      m_clientFd = clientFd;
    }
    handleClient(clientFd);
    {
      std::scoped_lock lock{m_clientMutex};
      m_clientFd = -1;
    }
    platformCloseSocket(clientFd);

    // The Mouser instance vanished: make sure the active client's virtual
    // device does not linger. Same path as an explicit disconnect line.
    if (m_running) {
      m_events->addEvent(Event(
          EventTypes::ServerMouserBridgeLine, m_eventTarget,
          new MouserBridgeLineData(R"({"type": "disconnect"})")
      ));
    }
  }
}

void MouserBridge::handleClient(int clientFd)
{
  std::string carry;
  if (!authenticate(clientFd, carry)) {
    return;
  }
  LOG_INFO("mouser bridge: local mouser connected");

  // Bring the fresh connection up to date with the current focus.
  std::string focusLine;
  {
    std::scoped_lock lock{m_focusMutex};
    focusLine = m_focusLine;
  }
  if (!focusLine.empty() && !sendLine(clientFd, focusLine)) {
    return;
  }

  std::string line;
  while (m_running && readLine(clientFd, carry, line)) {
    if (line.empty()) {
      continue;
    }
    m_events->addEvent(
        Event(EventTypes::ServerMouserBridgeLine, m_eventTarget, new MouserBridgeLineData(line))
    );
  }
  LOG_INFO("mouser bridge: local mouser disconnected");
}

bool MouserBridge::authenticate(int clientFd, std::string &carry)
{
  std::string line;
  if (!readLine(clientFd, carry, line)) {
    return false;
  }
  const QJsonObject hello = parseObject(line);
  const std::string token = hello[QStringLiteral("token")].toString().toStdString();
  if (hello[QStringLiteral("type")].toString() != QStringLiteral("hello") || !tokenMatches(m_token, token)) {
    LOG_WARN("mouser bridge: rejected unauthenticated connection");
    sendLine(clientFd, R"({"ok": false, "error": "unauthorized"})");
    return false;
  }
  return sendLine(clientFd, R"({"ok": true, "server": "deskflow-bridge", "version": 1})");
}

bool MouserBridge::readLine(int clientFd, std::string &carry, std::string &line) const
{
  while (true) {
    const auto newline = carry.find('\n');
    if (newline != std::string::npos) {
      line = carry.substr(0, newline);
      carry.erase(0, newline + 1);
      return true;
    }
    if (carry.size() > kMaxLineBytes) {
      LOG_WARN("mouser bridge: dropping oversized line");
      return false;
    }
    char buffer[4096];
    const auto received = ::recv(clientFd, buffer, sizeof(buffer), 0);
    if (received <= 0) {
      return false;
    }
    carry.append(buffer, static_cast<size_t>(received));
  }
}

bool MouserBridge::sendLine(int clientFd, const std::string &line)
{
  std::string payload = line;
  payload.push_back('\n');
  size_t sent = 0;
  while (sent < payload.size()) {
    const auto wrote = ::send(clientFd, payload.data() + sent, payload.size() - sent, 0);
    if (wrote <= 0) {
      return false;
    }
    sent += static_cast<size_t>(wrote);
  }
  return true;
}

void MouserBridge::closeFd(int &fd) const
{
  if (fd >= 0) {
    platformCloseSocket(fd);
    fd = -1;
  }
}
