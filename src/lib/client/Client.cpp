/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "client/Client.h"

#include "arch/Arch.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "client/ServerProxy.h"
#include "common/Settings.h"
#include "deskflow/Clipboard.h"
#include "deskflow/DeskflowException.h"
#include "deskflow/IPlatformScreen.h"
#include "deskflow/PacketStreamFilter.h"
#include "deskflow/ProtocolUtil.h"
#include "deskflow/Screen.h"
#include "deskflow/StreamChunker.h"
#include "net/IDataSocket.h"
#include "net/ISocketFactory.h"
#include "net/SecureSocket.h"
#include "net/TCPSocket.h"

#include <cstdlib>
#include <cstring>
#include <memory>

using namespace deskflow::client;

//
// Client
//

Client::Client(
    IEventQueue *events, const std::string &name, const NetworkAddress &address, ISocketFactory *socketFactory,
    deskflow::Screen *screen
)
    : m_name(name),
      m_serverAddress(address),
      m_socketFactory(socketFactory),
      m_screen(screen),
      m_events(events),
      m_useSecureNetwork(Settings::value(Settings::Security::TlsEnabled).toBool())
{
  assert(m_socketFactory != nullptr);
  assert(m_screen != nullptr);

  // register suspend/resume event handlers
  m_events->addHandler(EventTypes::ScreenSuspend, getEventTarget(), [this](const auto &) { handleSuspend(); });
  m_events->addHandler(EventTypes::ScreenResume, getEventTarget(), [this](const auto &) { handleResume(); });

  m_pHelloBack = std::make_unique<HelloBack>(std::make_shared<HelloBack::Deps>(
      [this]() {
        sendConnectionFailedEvent("got invalid hello message from server");
        cleanupTimer();
        cleanupConnection();
      },
      [this](int major, int minor) {
        sendConnectionFailedEvent(IncompatibleClientException(major, minor).what());
        cleanupTimer();
        cleanupConnection();
      }
  ));
}

Client::~Client()
{
  m_events->removeHandler(EventTypes::ScreenSuspend, getEventTarget());
  m_events->removeHandler(EventTypes::ScreenResume, getEventTarget());

  cleanupTimer();
  cleanupScreen();
  cleanupConnecting();
  cleanupConnection();
  delete m_socketFactory;
}

void Client::setServerAddress(const NetworkAddress &address)
{
  m_serverAddress = address;
}

void Client::connect(size_t addressIndex)
{
  if (m_stream != nullptr) {
    return;
  }
  if (m_suspended) {
    m_connectOnResume = true;
    return;
  }

  auto securityLevel = m_useSecureNetwork ? SecurityLevel::PeerAuth : SecurityLevel::PlainText;

  try {
    // resolve the server hostname.  do this every time we connect
    // in case we couldn't resolve the address earlier or the address
    // has changed (which can happen frequently if this is a laptop
    // being shuttled between various networks).  patch by Brent
    // Priddy.
    m_resolvedAddressesCount = m_serverAddress.resolve(addressIndex);

    // m_serverAddress will be null if the hostname address is not reolved
    if (m_serverAddress.getAddress() != nullptr) {
      // to help users troubleshoot, show server host name (issue: 60)
      LOG_IPC(
          "connecting to '%s': %s:%i", m_serverAddress.getHostname().c_str(),
          ARCH->addrToString(m_serverAddress.getAddress()).c_str(), m_serverAddress.getPort()
      );
    }

    // create the socket
    IDataSocket *socket = m_socketFactory->create(ARCH->getAddrFamily(m_serverAddress.getAddress()), securityLevel);
    bindNetworkInterface(socket);

    // filter socket messages, including a packetizing filter
    m_stream = new PacketStreamFilter(m_events, socket, true);

    // connect
    LOG_DEBUG1("connecting to server");
    setupConnecting();
    setupTimer();
    socket->connect(m_serverAddress);
  } catch (BaseException &e) {
    cleanupTimer();
    cleanupConnecting();
    cleanupStream();
    LOG_DEBUG1("connection failed");
    sendConnectionFailedEvent(e.what());
    return;
  }
}

void Client::disconnect(const char *msg)
{
  cleanup();

  if (msg) {
    sendConnectionFailedEvent(msg);
  } else {
    sendEvent(EventTypes::ClientDisconnected);
  }
}

void Client::refuseConnection(const char *msg)
{
  cleanup();

  if (msg) {
    auto info = new FailInfo(msg);
    info->m_retry = true;
    Event event(EventTypes::ClientConnectionRefused, getEventTarget(), info, Event::EventFlags::DontFreeData);
    m_events->addEvent(std::move(event));
  }
}

void Client::handshakeComplete()
{
  m_ready = true;
  m_screen->enable();
  sendEvent(EventTypes::ClientConnected);
}

bool Client::isConnected() const
{
  return (m_server != nullptr);
}

bool Client::isConnecting() const
{
  return (m_timer != nullptr);
}

NetworkAddress Client::getServerAddress() const
{
  return m_serverAddress;
}

void *Client::getEventTarget() const
{
  return m_screen->getEventTarget();
}

bool Client::getClipboard(ClipboardID id, IClipboard *clipboard) const
{
  return m_screen->getClipboard(id, clipboard);
}

void Client::getShape(int32_t &x, int32_t &y, int32_t &w, int32_t &h) const
{
  m_screen->getShape(x, y, w, h);
}

void Client::getCursorPos(int32_t &x, int32_t &y) const
{
  m_screen->getCursorPos(x, y);
}

void Client::enter(int32_t xAbs, int32_t yAbs, uint32_t, KeyModifierMask mask, bool)
{
  m_active = true;
  m_screen->mouseMove(xAbs, yAbs);
  m_screen->enter(mask);
}

bool Client::leave()
{
  m_active = false;

  m_screen->leave();

  if (m_enableClipboard) {
    // send clipboards that we own and that have changed
    for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
      if (m_ownClipboard[id]) {
        sendClipboard(id);
      }
    }
  }

  return true;
}

void Client::setClipboard(ClipboardID id, const IClipboard *clipboard)
{
  m_screen->setClipboard(id, clipboard);
  m_ownClipboard[id] = false;
  m_sentClipboard[id] = false;
}

void Client::grabClipboard(ClipboardID id)
{
  m_screen->grabClipboard(id);
  m_ownClipboard[id] = false;
  m_sentClipboard[id] = false;
}

void Client::setClipboardDirty(ClipboardID, bool)
{
  assert(0 && "shouldn't be called");
}

void Client::keyDown(KeyID id, KeyModifierMask mask, KeyButton button, const std::string &lang)
{
  m_screen->keyDown(id, mask, button, lang);
}

void Client::keyRepeat(KeyID id, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &lang)
{
  m_screen->keyRepeat(id, mask, count, button, lang);
}

void Client::keyUp(KeyID id, KeyModifierMask mask, KeyButton button)
{
  m_screen->keyUp(id, mask, button);
}

void Client::mouseDown(ButtonID id)
{
  m_screen->mouseDown(id);
}

void Client::mouseUp(ButtonID id)
{
  m_screen->mouseUp(id);
}

void Client::mouseMove(int32_t x, int32_t y)
{
  m_screen->mouseMove(x, y);
}

void Client::mouseRelativeMove(int32_t dx, int32_t dy)
{
  m_screen->mouseRelativeMove(dx, dy);
}

void Client::mouseWheel(int32_t xDelta, int32_t yDelta)
{
  m_screen->mouseWheel(xDelta, yDelta);
}

void Client::screensaver(bool activate)
{
  m_screen->screensaver(activate);
}

void Client::resetOptions()
{
  m_screen->resetOptions();
}

void Client::setOptions(const OptionsList &options)
{
  for (auto index = options.begin(); index != options.end(); ++index) {
    const OptionID id = *index;
    if (id == kOptionClipboardSharing) {
      index++;
      if (index != options.end()) {
        if (!*index) {
          LOG_NOTE("clipboard sharing disabled by server");
        }
        m_enableClipboard = *index;
      }
    } else if (id == kOptionClipboardSharingSize) {
      index++;
      if (index != options.end()) {
        m_maximumClipboardSize = *index;
      }
    }
  }

  if (m_enableClipboard && !m_maximumClipboardSize) {
    m_enableClipboard = false;
    LOG_NOTE("clipboard sharing is disabled because the server set the maximum clipboard size to 0");
  }

  m_screen->setOptions(options);
}

std::string Client::getName() const
{
  return m_name;
}

void Client::sendClipboard(ClipboardID id)
{
  // note -- m_mutex must be locked on entry
  assert(m_screen != nullptr);
  assert(m_server != nullptr);

  // get clipboard data.  set the clipboard time to the last
  // clipboard time before getting the data from the screen
  // as the screen may detect an unchanged clipboard and
  // avoid copying the data.
  Clipboard clipboard;
  if (clipboard.open(m_timeClipboard[id])) {
    clipboard.close();
  }
  m_screen->getClipboard(id, &clipboard);

  // check time
  if (m_timeClipboard[id] == 0 || clipboard.getTime() != m_timeClipboard[id]) {
    // marshall the data
    std::string data = clipboard.marshall();
    if (data.size() >= m_maximumClipboardSize * 1024) {
      LOG(
          (CLOG_NOTE "skipping clipboard transfer because the clipboard"
                     " contents exceeds the %i MB size limit set by the server",
           m_maximumClipboardSize / 1024)
      );
      return;
    }

    // save new time
    m_timeClipboard[id] = clipboard.getTime();
    // save and send data if different or not yet sent
    if (!m_sentClipboard[id] || data != m_dataClipboard[id]) {
      m_sentClipboard[id] = true;
      m_dataClipboard[id] = data;
      m_server->onClipboardChanged(id, &clipboard);
    }
  }
}

void Client::sendEvent(EventTypes type)
{
  m_events->addEvent(Event(type, getEventTarget()));
}

void Client::sendConnectionFailedEvent(const char *msg)
{
  auto *info = new FailInfo(msg);
  info->m_retry = true;
  Event event(EventTypes::ClientConnectionFailed, getEventTarget(), info, Event::EventFlags::DontFreeData);
  m_events->addEvent(std::move(event));
}

void Client::setupConnecting()
{
  assert(m_stream != nullptr);

  if (Settings::value(Settings::Security::TlsEnabled).toBool()) {
    m_events->addHandler(EventTypes::DataSocketSecureConnected, m_stream->getEventTarget(), [this](const auto &) {
      handleConnected();
    });
  } else {
    m_events->addHandler(EventTypes::DataSocketConnected, m_stream->getEventTarget(), [this](const auto &) {
      handleConnected();
    });
  }
  m_events->addHandler(EventTypes::DataSocketConnectionFailed, m_stream->getEventTarget(), [this](const auto &e) {
    handleConnectionFailed(e);
  });
}

void Client::setupConnection()
{
  assert(m_stream != nullptr);

  m_events->addHandler(EventTypes::SocketDisconnected, m_stream->getEventTarget(), [this](const auto &) {
    handleDisconnected();
  });
  m_events->addHandler(EventTypes::StreamInputReady, m_stream->getEventTarget(), [this](const auto &) {
    handleHello();
  });
  m_events->addHandler(EventTypes::StreamOutputError, m_stream->getEventTarget(), [this](const auto &) {
    handleOutputError();
  });
  m_events->addHandler(EventTypes::StreamInputShutdown, m_stream->getEventTarget(), [this](const auto &) {
    handleDisconnected();
  });
  m_events->addHandler(EventTypes::StreamOutputShutdown, m_stream->getEventTarget(), [this](const auto &) {
    handleDisconnected();
  });
}

void Client::setupScreen()
{
  assert(m_server == nullptr);

  m_ready = false;
  m_server = new ServerProxy(this, m_stream, m_events);
  m_events->addHandler(EventTypes::ScreenShapeChanged, getEventTarget(), [this](const auto &) {
    handleShapeChanged();
  });
  m_events->addHandler(EventTypes::ClipboardGrabbed, getEventTarget(), [this](const auto &e) {
    handleClipboardGrabbed(e);
  });
}

void Client::setupTimer()
{
  assert(m_timer == nullptr);
  m_timer = m_events->newOneShotTimer(2.0, nullptr);
  m_events->addHandler(EventTypes::Timer, m_timer, [this](const auto &) { handleConnectTimeout(); });
}

void Client::cleanup()
{
  m_connectOnResume = false;
  cleanupTimer();
  cleanupScreen();
  cleanupConnecting();
  cleanupConnection();
}

void Client::cleanupConnecting()
{
  if (m_stream != nullptr) {
    m_events->removeHandler(EventTypes::DataSocketConnected, m_stream->getEventTarget());
    m_events->removeHandler(EventTypes::DataSocketConnectionFailed, m_stream->getEventTarget());
  }
}

void Client::cleanupConnection()
{
  if (m_stream != nullptr) {
    using enum EventTypes;
    m_events->removeHandler(StreamInputReady, m_stream->getEventTarget());
    m_events->removeHandler(StreamOutputError, m_stream->getEventTarget());
    m_events->removeHandler(StreamInputShutdown, m_stream->getEventTarget());
    m_events->removeHandler(StreamOutputShutdown, m_stream->getEventTarget());
    m_events->removeHandler(SocketDisconnected, m_stream->getEventTarget());
    cleanupStream();
  }
}

void Client::cleanupScreen()
{
  if (m_server != nullptr) {
    if (m_ready) {
      m_screen->disable();
      m_ready = false;
    }
    m_events->removeHandler(EventTypes::ScreenShapeChanged, getEventTarget());
    m_events->removeHandler(EventTypes::ClipboardGrabbed, getEventTarget());
    delete m_server;
    m_server = nullptr;
  }
}

void Client::cleanupTimer()
{
  if (m_timer != nullptr) {
    m_events->removeHandler(EventTypes::Timer, m_timer);
    m_events->deleteTimer(m_timer);
    m_timer = nullptr;
  }
}

void Client::cleanupStream()
{
  delete m_stream;
  m_stream = nullptr;
}

void Client::handleConnected()
{
  LOG_DEBUG1("connected, waiting for hello");
  cleanupConnecting();
  setupConnection();

  // reset clipboard state
  for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
    m_ownClipboard[id] = false;
    m_sentClipboard[id] = false;
    m_timeClipboard[id] = 0;
  }
}

void Client::handleConnectionFailed(const Event &event)
{
  auto *info = static_cast<IDataSocket::ConnectionFailedInfo *>(event.getData());

  cleanupTimer();
  cleanupConnecting();
  cleanupStream();
  LOG_DEBUG1("connection failed");
  sendConnectionFailedEvent(info->m_what.c_str());
  delete info;
}

void Client::handleConnectTimeout()
{
  cleanupTimer();
  cleanupConnecting();
  cleanupConnection();
  cleanupStream();
  LOG_DEBUG1("connection timed out");
  sendConnectionFailedEvent("Timed out");
}

void Client::handleOutputError()
{
  cleanupTimer();
  cleanupScreen();
  cleanupConnection();
  LOG_WARN("error sending to server");
  sendEvent(EventTypes::ClientDisconnected);
}

void Client::handleDisconnected()
{
  cleanupTimer();
  cleanupScreen();
  cleanupConnection();
  LOG_DEBUG1("disconnected");
  sendEvent(EventTypes::ClientDisconnected);
}

void Client::handleShapeChanged()
{
  LOG_DEBUG("resolution changed");
  m_server->onInfoChanged();
}

void Client::handleClipboardGrabbed(const Event &event)
{
  if (!m_enableClipboard || (m_maximumClipboardSize == 0)) {
    return;
  }

  const auto *info = static_cast<const IScreen::ClipboardInfo *>(event.getData());

  // grab ownership
  m_server->onGrabClipboard(info->m_id);

  // we now own the clipboard and it has not been sent to the server
  m_ownClipboard[info->m_id] = true;
  m_sentClipboard[info->m_id] = false;
  m_timeClipboard[info->m_id] = 0;

  // if we're not the active screen then send the clipboard now,
  // otherwise we'll wait until we leave.
  if (!m_active) {
    sendClipboard(info->m_id);
  }
}

void Client::handleHello()
{
  m_pHelloBack->handleHello(m_stream, m_name);

  // now connected but waiting to complete handshake
  setupScreen();
  cleanupTimer();

  // make sure we process any remaining messages later.  we won't
  // receive another event for already pending messages so we fake
  // one.
  if (m_stream->isReady()) {
    m_events->addEvent(Event(EventTypes::StreamInputReady, m_stream->getEventTarget()));
  }
}

void Client::handleSuspend()
{
  if (!m_suspended) {
    LOG_INFO("suspend");
    m_suspended = true;
    bool wasConnected = isConnected();
    disconnect(nullptr);
    m_connectOnResume = wasConnected;
  }
}

void Client::handleResume()
{
  if (m_suspended) {
    LOG_INFO("resume");
    m_suspended = false;
    if (m_connectOnResume) {
      m_connectOnResume = false;
      connect();
    }
  }
}

void Client::bindNetworkInterface(IDataSocket *socket) const
{
  try {
    if (const auto address = Settings::value(Settings::Core::Interface).toString(); !address.isEmpty()) {
      LOG_DEBUG1("bind to network interface: %s", qPrintable(address));

      NetworkAddress bindAddress(address.toStdString());
      bindAddress.resolve();

      socket->bind(bindAddress);
    }
  } catch (BaseException &e) {
    LOG_WARN("%s", e.what());
    LOG_WARN("operating system will select network interface automatically");
  }
}
