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
#include "base/TMethodEventJob.h"
#include "base/TMethodJob.h"
#include "client/ServerProxy.h"
#include "common/stdexcept.h"
#include "deskflow/AppUtil.h"
#include "deskflow/DropHelper.h"
#include "deskflow/FileChunk.h"
#include "deskflow/IPlatformScreen.h"
#include "deskflow/PacketStreamFilter.h"
#include "deskflow/ProtocolUtil.h"
#include "deskflow/Screen.h"
#include "deskflow/StreamChunker.h"
#include "deskflow/XDeskflow.h"
#include "deskflow/protocol_types.h"
#include "mt/Thread.h"
#include "net/IDataSocket.h"
#include "net/ISocketFactory.h"
#include "net/SecureSocket.h"
#include "net/TCPSocket.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <sstream>

using namespace deskflow::client;

//
// Client
//

Client::Client(
    IEventQueue *events, const std::string &name, const NetworkAddress &address, ISocketFactory *socketFactory,
    deskflow::Screen *screen, deskflow::ClientArgs const &args
)
    : m_mock(false),
      m_name(name),
      m_serverAddress(address),
      m_socketFactory(socketFactory),
      m_screen(screen),
      m_stream(NULL),
      m_timer(NULL),
      m_server(NULL),
      m_ready(false),
      m_active(false),
      m_suspended(false),
      m_connectOnResume(false),
      m_events(events),
      m_sendFileThread(nullptr),
      m_writeToDropDirThread(nullptr),
      m_useSecureNetwork(args.m_enableCrypto),
      m_args(args),
      m_enableClipboard(true),
      m_maximumClipboardSize(INT_MAX)
{
  assert(m_socketFactory != NULL);
  assert(m_screen != NULL);

  // register suspend/resume event handlers
  m_events->adoptHandler(
      m_events->forIScreen().suspend(), getEventTarget(), new TMethodEventJob<Client>(this, &Client::handleSuspend)
  );
  m_events->adoptHandler(
      m_events->forIScreen().resume(), getEventTarget(), new TMethodEventJob<Client>(this, &Client::handleResume)
  );

  if (m_args.m_enableDragDrop) {
    m_events->adoptHandler(
        m_events->forFile().fileChunkSending(), this, new TMethodEventJob<Client>(this, &Client::handleFileChunkSending)
    );
    m_events->adoptHandler(
        m_events->forFile().fileRecieveCompleted(), this,
        new TMethodEventJob<Client>(this, &Client::handleFileRecieveCompleted)
    );
  }

  m_pHelloBack = std::make_unique<HelloBack>(std::make_shared<HelloBack::Deps>(
      [this]() {
        sendConnectionFailedEvent("got invalid hello message from server");
        cleanupTimer();
        cleanupConnection();
      },
      [this](int major, int minor) {
        sendConnectionFailedEvent(XIncompatibleClient(major, minor).what());
        cleanupTimer();
        cleanupConnection();
      }
  ));
}

Client::~Client()
{
  if (m_mock) {
    return;
  }

  m_events->removeHandler(m_events->forIScreen().suspend(), getEventTarget());
  m_events->removeHandler(m_events->forIScreen().resume(), getEventTarget());

  cleanupTimer();
  cleanupScreen();
  cleanupConnecting();
  cleanupConnection();
  delete m_socketFactory;
}

void Client::connect(size_t addressIndex)
{
  if (m_stream != NULL) {
    return;
  }
  if (m_suspended) {
    m_connectOnResume = true;
    return;
  }

  auto securityLevel = m_useSecureNetwork ? SecurityLevel::PeerAuth : SecurityLevel::PlainText;

  try {
    if (m_args.m_hostMode) {
      LOG((CLOG_NOTE "waiting for server connection on %i port", m_serverAddress.getPort()));
    } else {
      // resolve the server hostname.  do this every time we connect
      // in case we couldn't resolve the address earlier or the address
      // has changed (which can happen frequently if this is a laptop
      // being shuttled between various networks).  patch by Brent
      // Priddy.
      m_resolvedAddressesCount = m_serverAddress.resolve(addressIndex);

      // m_serverAddress will be null if the hostname address is not reolved
      if (m_serverAddress.getAddress() != nullptr) {
        // to help users troubleshoot, show server host name (issue: 60)
        LOG(
            (CLOG_NOTE "connecting to '%s': %s:%i", m_serverAddress.getHostname().c_str(),
             ARCH->addrToString(m_serverAddress.getAddress()).c_str(), m_serverAddress.getPort())
        );
      }
    }

    // create the socket
    IDataSocket *socket = m_socketFactory->create(ARCH->getAddrFamily(m_serverAddress.getAddress()), securityLevel);
    bindNetworkInterface(socket);

    // filter socket messages, including a packetizing filter
    m_stream = new PacketStreamFilter(m_events, socket, true);

    // connect
    LOG((CLOG_DEBUG1 "connecting to server"));
    setupConnecting();
    setupTimer();
    socket->connect(m_serverAddress);
  } catch (XBase &e) {
    cleanupTimer();
    cleanupConnecting();
    cleanupStream();
    LOG((CLOG_DEBUG1 "connection failed"));
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
    sendEvent(m_events->forClient().disconnected(), NULL);
  }
}

void Client::refuseConnection(const char *msg)
{
  cleanup();

  if (msg) {
    auto info = new FailInfo(msg);
    info->m_retry = true;
    Event event(m_events->forClient().connectionRefused(), getEventTarget(), info, Event::kDontFreeData);
    m_events->addEvent(event);
  }
}

void Client::handshakeComplete()
{
  m_ready = true;
  m_screen->enable();
  sendEvent(m_events->forClient().connected(), NULL);
}

bool Client::isConnected() const
{
  return (m_server != NULL);
}

bool Client::isConnecting() const
{
  return (m_timer != NULL);
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

  if (m_sendFileThread) {
    StreamChunker::interruptFile();
    m_sendFileThread.reset(nullptr);
  }
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
  for (OptionsList::const_iterator index = options.begin(); index != options.end(); ++index) {
    const OptionID id = *index;
    if (id == kOptionClipboardSharing) {
      index++;
      if (index != options.end()) {
        if (!*index) {
          LOG((CLOG_NOTE "clipboard sharing disabled by server"));
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
    LOG((CLOG_NOTE "clipboard sharing is disabled because the server "
                   "set the maximum clipboard size to 0"));
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
  assert(m_screen != NULL);
  assert(m_server != NULL);

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

void Client::sendEvent(Event::Type type, void *data)
{
  m_events->addEvent(Event(type, getEventTarget(), data));
}

void Client::sendConnectionFailedEvent(const char *msg)
{
  FailInfo *info = new FailInfo(msg);
  info->m_retry = true;
  Event event(m_events->forClient().connectionFailed(), getEventTarget(), info, Event::kDontFreeData);
  m_events->addEvent(event);
}

void Client::sendFileChunk(const void *data)
{
  FileChunk *chunk = static_cast<FileChunk *>(const_cast<void *>(data));
  LOG((CLOG_DEBUG1 "send file chunk"));
  assert(m_server != NULL);

  // relay
  m_server->fileChunkSending(chunk->m_chunk[0], &chunk->m_chunk[1], chunk->m_dataSize);
}

void Client::setupConnecting()
{
  assert(m_stream != NULL);

  if (m_args.m_enableCrypto) {
    m_events->adoptHandler(
        m_events->forIDataSocket().secureConnected(), m_stream->getEventTarget(),
        new TMethodEventJob<Client>(this, &Client::handleConnected)
    );
  } else {
    m_events->adoptHandler(
        m_events->forIDataSocket().connected(), m_stream->getEventTarget(),
        new TMethodEventJob<Client>(this, &Client::handleConnected)
    );
  }

  m_events->adoptHandler(
      m_events->forIDataSocket().connectionFailed(), m_stream->getEventTarget(),
      new TMethodEventJob<Client>(this, &Client::handleConnectionFailed)
  );
}

void Client::setupConnection()
{
  assert(m_stream != NULL);

  m_events->adoptHandler(
      m_events->forISocket().disconnected(), m_stream->getEventTarget(),
      new TMethodEventJob<Client>(this, &Client::handleDisconnected)
  );
  m_events->adoptHandler(
      m_events->forIStream().inputReady(), m_stream->getEventTarget(),
      new TMethodEventJob<Client>(this, &Client::handleHello)
  );
  m_events->adoptHandler(
      m_events->forIStream().outputError(), m_stream->getEventTarget(),
      new TMethodEventJob<Client>(this, &Client::handleOutputError)
  );
  m_events->adoptHandler(
      m_events->forIStream().inputShutdown(), m_stream->getEventTarget(),
      new TMethodEventJob<Client>(this, &Client::handleDisconnected)
  );
  m_events->adoptHandler(
      m_events->forIStream().outputShutdown(), m_stream->getEventTarget(),
      new TMethodEventJob<Client>(this, &Client::handleDisconnected)
  );

  m_events->adoptHandler(
      m_events->forISocket().stopRetry(), m_stream->getEventTarget(),
      new TMethodEventJob<Client>(this, &Client::handleStopRetry)
  );
}

void Client::setupScreen()
{
  assert(m_server == NULL);

  m_ready = false;
  m_server = new ServerProxy(this, m_stream, m_events);
  m_events->adoptHandler(
      m_events->forIScreen().shapeChanged(), getEventTarget(),
      new TMethodEventJob<Client>(this, &Client::handleShapeChanged)
  );
  m_events->adoptHandler(
      m_events->forClipboard().clipboardGrabbed(), getEventTarget(),
      new TMethodEventJob<Client>(this, &Client::handleClipboardGrabbed)
  );
}

void Client::setupTimer()
{
  assert(m_timer == NULL);

  if (!m_args.m_hostMode) {
    m_timer = m_events->newOneShotTimer(2.0, NULL);
    m_events->adoptHandler(Event::kTimer, m_timer, new TMethodEventJob<Client>(this, &Client::handleConnectTimeout));
  }
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
  if (m_stream != NULL) {
    m_events->removeHandler(m_events->forIDataSocket().connected(), m_stream->getEventTarget());
    m_events->removeHandler(m_events->forIDataSocket().connectionFailed(), m_stream->getEventTarget());
  }
}

void Client::cleanupConnection()
{
  if (m_stream != NULL) {
    m_events->removeHandler(m_events->forIStream().inputReady(), m_stream->getEventTarget());
    m_events->removeHandler(m_events->forIStream().outputError(), m_stream->getEventTarget());
    m_events->removeHandler(m_events->forIStream().inputShutdown(), m_stream->getEventTarget());
    m_events->removeHandler(m_events->forIStream().outputShutdown(), m_stream->getEventTarget());
    m_events->removeHandler(m_events->forISocket().disconnected(), m_stream->getEventTarget());
    m_events->removeHandler(m_events->forISocket().stopRetry(), m_stream->getEventTarget());
    cleanupStream();
  }
}

void Client::cleanupScreen()
{
  if (m_server != NULL) {
    if (m_ready) {
      m_screen->disable();
      m_ready = false;
    }
    m_events->removeHandler(m_events->forIScreen().shapeChanged(), getEventTarget());
    m_events->removeHandler(m_events->forClipboard().clipboardGrabbed(), getEventTarget());
    delete m_server;
    m_server = NULL;
  }
}

void Client::cleanupTimer()
{
  if (m_timer != NULL) {
    m_events->removeHandler(Event::kTimer, m_timer);
    m_events->deleteTimer(m_timer);
    m_timer = NULL;
  }
}

void Client::cleanupStream()
{
  delete m_stream;
  m_stream = NULL;
}

void Client::handleConnected(const Event &, void *)
{
  LOG((CLOG_DEBUG1 "connected, waiting for hello"));
  cleanupConnecting();
  setupConnection();

  // reset clipboard state
  for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
    m_ownClipboard[id] = false;
    m_sentClipboard[id] = false;
    m_timeClipboard[id] = 0;
  }
}

void Client::handleConnectionFailed(const Event &event, void *)
{
  IDataSocket::ConnectionFailedInfo *info = static_cast<IDataSocket::ConnectionFailedInfo *>(event.getData());

  cleanupTimer();
  cleanupConnecting();
  cleanupStream();
  LOG((CLOG_DEBUG1 "connection failed"));
  sendConnectionFailedEvent(info->m_what.c_str());
  delete info;
}

void Client::handleConnectTimeout(const Event &, void *)
{
  cleanupTimer();
  cleanupConnecting();
  cleanupConnection();
  cleanupStream();
  LOG((CLOG_DEBUG1 "connection timed out"));
  sendConnectionFailedEvent("Timed out");
}

void Client::handleOutputError(const Event &, void *)
{
  cleanupTimer();
  cleanupScreen();
  cleanupConnection();
  LOG((CLOG_WARN "error sending to server"));
  sendEvent(m_events->forClient().disconnected(), NULL);
}

void Client::handleDisconnected(const Event &, void *)
{
  cleanupTimer();
  cleanupScreen();
  cleanupConnection();
  LOG((CLOG_DEBUG1 "disconnected"));
  sendEvent(m_events->forClient().disconnected(), NULL);
}

void Client::handleShapeChanged(const Event &, void *)
{
  LOG((CLOG_DEBUG "resolution changed"));
  m_server->onInfoChanged();
}

void Client::handleClipboardGrabbed(const Event &event, void *)
{
  if (!m_enableClipboard || (m_maximumClipboardSize == 0)) {
    return;
  }

  const IScreen::ClipboardInfo *info = static_cast<const IScreen::ClipboardInfo *>(event.getData());

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

void Client::handleHello(const Event &, void *)
{
  m_pHelloBack->handleHello(m_stream, m_name);

  // now connected but waiting to complete handshake
  setupScreen();
  cleanupTimer();

  // make sure we process any remaining messages later.  we won't
  // receive another event for already pending messages so we fake
  // one.
  if (m_stream->isReady()) {
    m_events->addEvent(Event(m_events->forIStream().inputReady(), m_stream->getEventTarget()));
  }
}

void Client::handleSuspend(const Event &, void *)
{
  if (!m_suspended) {
    LOG((CLOG_INFO "suspend"));
    m_suspended = true;
    bool wasConnected = isConnected();
    disconnect(NULL);
    m_connectOnResume = wasConnected;
  }
}

void Client::handleResume(const Event &, void *)
{
  if (m_suspended) {
    LOG((CLOG_INFO "resume"));
    m_suspended = false;
    if (m_connectOnResume) {
      m_connectOnResume = false;
      connect();
    }
  }
}

void Client::handleFileChunkSending(const Event &event, void *)
{
  sendFileChunk(event.getDataObject());
}

void Client::handleFileRecieveCompleted(const Event &event, void *)
{
  onFileRecieveCompleted();
}

void Client::onFileRecieveCompleted()
{
  if (isReceivedFileSizeValid()) {
    auto method = new TMethodJob<Client>(this, &Client::writeToDropDirThread);
    m_writeToDropDirThread.reset(new Thread(method));
  }
}

void Client::bindNetworkInterface(IDataSocket *socket) const
{
  try {
    if (!m_args.m_deskflowAddress.empty()) {
      LOG((CLOG_DEBUG1 "bind to network interface: %s", m_args.m_deskflowAddress.c_str()));

      NetworkAddress bindAddress(m_args.m_deskflowAddress);
      bindAddress.resolve();

      socket->bind(bindAddress);
    }
  } catch (XBase &e) {
    LOG((CLOG_WARN "%s", e.what()));
    LOG((CLOG_WARN "operating system will select network interface automatically"));
  }
}

void Client::handleStopRetry(const Event &, void *)
{
  m_args.m_restartable = false;
}

void Client::writeToDropDirThread(void *)
{
  LOG((CLOG_DEBUG "starting write to drop dir thread"));

  while (m_screen->isFakeDraggingStarted()) {
    ARCH->sleep(.1f);
  }

  DropHelper::writeToDir(m_screen->getDropTarget(), m_dragFileList, m_receivedFileData);
}

void Client::dragInfoReceived(uint32_t fileNum, std::string data)
{
  // TODO: fix duplicate function from CServer
  if (!m_args.m_enableDragDrop) {
    LOG((CLOG_DEBUG "drag drop not enabled, ignoring drag info."));
    return;
  }

  DragInformation::parseDragInfo(m_dragFileList, fileNum, data);

  m_screen->startDraggingFiles(m_dragFileList);
}

bool Client::isReceivedFileSizeValid()
{
  return m_expectedFileSize == m_receivedFileData.size();
}

void Client::sendFileToServer(const char *filename)
{
  if (m_sendFileThread) {
    StreamChunker::interruptFile();
  }

  auto data = static_cast<void *>(const_cast<char *>(filename));
  auto method = new TMethodJob<Client>(this, &Client::sendFileThread, data);
  m_sendFileThread.reset(new Thread(method));
}

void Client::sendFileThread(void *filename)
{
  try {
    char *name = static_cast<char *>(filename);
    StreamChunker::sendFile(name, m_events, this);
  } catch (std::runtime_error &error) {
    LOG((CLOG_ERR "failed sending file chunks: %s", error.what()));
  }

  m_sendFileThread.reset(nullptr);
}

void Client::sendDragInfo(uint32_t fileCount, std::string &info, size_t size)
{
  m_server->sendDragInfo(fileCount, info.c_str(), size);
}
