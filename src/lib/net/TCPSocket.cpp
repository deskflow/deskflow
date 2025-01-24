/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/TCPSocket.h"

#include "arch/Arch.h"
#include "arch/XArch.h"
#include "base/IEventJob.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "mt/Lock.h"
#include "net/NetworkAddress.h"
#include "net/SocketMultiplexer.h"
#include "net/TSocketMultiplexerMethodJob.h"
#include "net/XSocket.h"

#include <cstdlib>
#include <cstring>
#include <memory>

static const std::size_t MAX_INPUT_BUFFER_SIZE = 1024 * 1024;

//
// TCPSocket
//

TCPSocket::TCPSocket(IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family)
    : IDataSocket(events),
      m_events(events),
      m_mutex(),
      m_flushed(&m_mutex, true),
      m_socketMultiplexer(socketMultiplexer)
{
  try {
    m_socket = ARCH->newSocket(family, IArchNetwork::kSTREAM);
  } catch (const XArchNetwork &e) {
    throw XSocketCreate(e.what());
  }

  LOG((CLOG_DEBUG "opening new socket: %08X", m_socket));

  init();
}

TCPSocket::TCPSocket(IEventQueue *events, SocketMultiplexer *socketMultiplexer, ArchSocket socket)
    : IDataSocket(events),
      m_events(events),
      m_mutex(),
      m_socket(socket),
      m_flushed(&m_mutex, true),
      m_socketMultiplexer(socketMultiplexer)
{
  assert(m_socket != nullptr);

  LOG((CLOG_DEBUG "opening new socket: %08X", m_socket));

  // socket starts in connected state
  init();
  onConnected();
  setJob(newJob());
}

TCPSocket::~TCPSocket()
{
  try {
    // warning virtual function in destructor is very danger practice
    close();
  } catch (...) {
    LOG((CLOG_DEBUG "error while TCP socket destruction"));
  }
}

void TCPSocket::bind(const NetworkAddress &addr)
{
  try {
    ARCH->bindSocket(m_socket, addr.getAddress());
  } catch (const XArchNetworkAddressInUse &e) {
    throw XSocketAddressInUse(e.what());
  } catch (const XArchNetwork &e) {
    throw XSocketBind(e.what());
  }
}

void TCPSocket::close()
{
  LOG((CLOG_DEBUG "closing socket: %08X", m_socket));

  // remove ourself from the multiplexer
  setJob(nullptr);

  Lock lock(&m_mutex);

  // clear buffers and enter disconnected state
  if (m_connected) {
    sendEvent(m_events->forISocket().disconnected());
  }
  onDisconnected();

  // close the socket
  if (m_socket != nullptr) {
    ArchSocket socket = m_socket;
    m_socket = nullptr;
    try {
      ARCH->closeSocket(socket);
    } catch (const XArchNetwork &e) {
      // ignore, there's not much we can do
      LOG((CLOG_WARN "error closing socket: %s", e.what()));
    }
  }
}

void *TCPSocket::getEventTarget() const
{
  return const_cast<void *>(static_cast<const void *>(this));
}

uint32_t TCPSocket::read(void *buffer, uint32_t n)
{
  // copy data directly from our input buffer
  Lock lock(&m_mutex);
  uint32_t size = m_inputBuffer.getSize();
  if (n > size) {
    n = size;
  }
  if (buffer != nullptr && n != 0) {
    memcpy(buffer, m_inputBuffer.peek(n), n);
  }
  m_inputBuffer.pop(n);

  // if no more data and we cannot read or write then send disconnected
  if (n > 0 && m_inputBuffer.getSize() == 0 && !m_readable && !m_writable) {
    sendEvent(m_events->forISocket().disconnected());
    m_connected = false;
  }

  return n;
}

void TCPSocket::write(const void *buffer, uint32_t n)
{
  bool wasEmpty;
  {
    Lock lock(&m_mutex);

    // must not have shutdown output
    if (!m_writable) {
      sendEvent(m_events->forIStream().outputError());
      return;
    }

    // ignore empty writes
    if (n == 0) {
      return;
    }

    // copy data to the output buffer
    wasEmpty = (m_outputBuffer.getSize() == 0);
    m_outputBuffer.write(buffer, n);

    // there's data to write
    m_flushed = false;
  }

  // make sure we're waiting to write
  if (wasEmpty) {
    setJob(newJob());
  }
}

void TCPSocket::flush()
{
  Lock lock(&m_mutex);
  while (m_flushed == false) {
    m_flushed.wait();
  }
}

void TCPSocket::shutdownInput()
{
  bool useNewJob = false;
  {
    Lock lock(&m_mutex);

    // shutdown socket for reading
    try {
      ARCH->closeSocketForRead(m_socket);
    } catch (const XArchNetwork &e) {
      // ignore, there's not much we can do
      LOG((CLOG_WARN "error closing socket: %s", e.what()));
    }

    // shutdown buffer for reading
    if (m_readable) {
      sendEvent(m_events->forIStream().inputShutdown());
      onInputShutdown();
      useNewJob = true;
    }
  }
  if (useNewJob) {
    setJob(newJob());
  }
}

void TCPSocket::shutdownOutput()
{
  bool useNewJob = false;
  {
    Lock lock(&m_mutex);

    // shutdown socket for writing
    try {
      ARCH->closeSocketForWrite(m_socket);
    } catch (const XArchNetwork &e) {
      // ignore, there's not much we can do
      LOG((CLOG_WARN "error closing socket: %s", e.what()));
    }

    // shutdown buffer for writing
    if (m_writable) {
      sendEvent(m_events->forIStream().outputShutdown());
      onOutputShutdown();
      useNewJob = true;
    }
  }
  if (useNewJob) {
    setJob(newJob());
  }
}

bool TCPSocket::isReady() const
{
  Lock lock(&m_mutex);
  return (m_inputBuffer.getSize() > 0);
}

bool TCPSocket::isFatal() const
{
  // TCP sockets aren't ever left in a fatal state.
  LOG((CLOG_ERR "isFatal() not valid for non-secure connections"));
  return false;
}

uint32_t TCPSocket::getSize() const
{
  Lock lock(&m_mutex);
  return m_inputBuffer.getSize();
}

void TCPSocket::connect(const NetworkAddress &addr)
{
  {
    Lock lock(&m_mutex);

    // fail on attempts to reconnect
    if (m_socket == nullptr || m_connected) {
      sendConnectionFailedEvent("busy");
      return;
    }

    try {
      if (ARCH->connectSocket(m_socket, addr.getAddress())) {
        sendEvent(m_events->forIDataSocket().connected());
        onConnected();
      } else {
        // connection is in progress
        m_writable = true;
      }
    } catch (const XArchNetwork &e) {
      throw XSocketConnect(e.what());
    }
  }
  setJob(newJob());
}

void TCPSocket::init()
{
  // default state
  m_connected = false;
  m_readable = false;
  m_writable = false;

  try {
    // turn off Nagle algorithm.  we send lots of very short messages
    // that should be sent without (much) delay.  for example, the
    // mouse motion messages are much less useful if they're delayed.
    ARCH->setNoDelayOnSocket(m_socket, true);
  } catch (const XArchNetwork &e) {
    try {
      ARCH->closeSocket(m_socket);
      m_socket = nullptr;
    } catch (const XArchNetwork &e) {
      // ignore, there's not much we can do
      LOG((CLOG_WARN "error closing socket: %s", e.what()));
    }
    throw XSocketCreate(e.what());
  }
}

TCPSocket::EJobResult TCPSocket::doRead()
{
  uint8_t buffer[4096];
  memset(buffer, 0, sizeof(buffer));
  size_t bytesRead = 0;

  bytesRead = ARCH->readSocket(m_socket, buffer, sizeof(buffer));

  if (bytesRead > 0) {
    bool wasEmpty = (m_inputBuffer.getSize() == 0);

    // slurp up as much as possible
    do {
      m_inputBuffer.write(buffer, static_cast<uint32_t>(bytesRead));

      if (m_inputBuffer.getSize() > MAX_INPUT_BUFFER_SIZE) {
        break;
      }

      bytesRead = ARCH->readSocket(m_socket, buffer, sizeof(buffer));
    } while (bytesRead > 0);

    // send input ready if input buffer was empty
    if (wasEmpty) {
      sendEvent(m_events->forIStream().inputReady());
    }
  } else {
    // remote write end of stream hungup.  our input side
    // has therefore shutdown but don't flush our buffer
    // since there's still data to be read.
    sendEvent(m_events->forIStream().inputShutdown());
    if (!m_writable && m_inputBuffer.getSize() == 0) {
      sendEvent(m_events->forISocket().disconnected());
      m_connected = false;
    }
    m_readable = false;
    return kNew;
  }

  return kRetry;
}

TCPSocket::EJobResult TCPSocket::doWrite()
{
  // write data
  uint32_t bufferSize = 0;
  int bytesWrote = 0;

  bufferSize = m_outputBuffer.getSize();
  const void *buffer = m_outputBuffer.peek(bufferSize);
  bytesWrote = (uint32_t)ARCH->writeSocket(m_socket, buffer, bufferSize);

  if (bytesWrote > 0) {
    discardWrittenData(bytesWrote);
    return kNew;
  }

  return kRetry;
}

void TCPSocket::setJob(ISocketMultiplexerJob *job)
{
  // multiplexer will delete the old job
  if (job == nullptr) {
    m_socketMultiplexer->removeSocket(this);
  } else {
    m_socketMultiplexer->addSocket(this, job);
  }
}

ISocketMultiplexerJob *TCPSocket::newJob()
{
  // note -- must have m_mutex locked on entry

  if (m_socket == nullptr) {
    return nullptr;
  } else if (!m_connected) {
    assert(!m_readable);
    if (!(m_readable || m_writable)) {
      return nullptr;
    }
    return new TSocketMultiplexerMethodJob<TCPSocket>(
        this, &TCPSocket::serviceConnecting, m_socket, m_readable, m_writable
    );
  } else {
    if (!(m_readable || (m_writable && (m_outputBuffer.getSize() > 0)))) {
      return nullptr;
    }
    return new TSocketMultiplexerMethodJob<TCPSocket>(
        this, &TCPSocket::serviceConnected, m_socket, m_readable, m_writable && (m_outputBuffer.getSize() > 0)
    );
  }
}

void TCPSocket::sendConnectionFailedEvent(const char *msg)
{
  ConnectionFailedInfo *info = new ConnectionFailedInfo(msg);
  m_events->addEvent(Event(m_events->forIDataSocket().connectionFailed(), getEventTarget(), info, Event::kDontFreeData)
  );
}

void TCPSocket::sendEvent(Event::Type type)
{
  m_events->addEvent(Event(type, getEventTarget()));
}

void TCPSocket::discardWrittenData(int bytesWrote)
{
  m_outputBuffer.pop(bytesWrote);
  if (m_outputBuffer.getSize() == 0) {
    sendEvent(m_events->forIStream().outputFlushed());
    m_flushed = true;
    m_flushed.broadcast();
  }
}

void TCPSocket::onConnected()
{
  m_connected = true;
  m_readable = true;
  m_writable = true;
}

void TCPSocket::onInputShutdown()
{
  m_inputBuffer.pop(m_inputBuffer.getSize());
  m_readable = false;
}

void TCPSocket::onOutputShutdown()
{
  m_outputBuffer.pop(m_outputBuffer.getSize());
  m_writable = false;

  // we're now flushed
  m_flushed = true;
  m_flushed.broadcast();
}

void TCPSocket::onDisconnected()
{
  // disconnected
  onInputShutdown();
  onOutputShutdown();
  m_connected = false;
}

ISocketMultiplexerJob *TCPSocket::serviceConnecting(ISocketMultiplexerJob *job, bool, bool write, bool error)
{
  Lock lock(&m_mutex);

  // should only check for errors if error is true but checking a new
  // socket (and a socket that's connecting should be new) for errors
  // should be safe and Mac OS X appears to have a bug where a
  // non-blocking stream socket that fails to connect immediately is
  // reported by select as being writable (i.e. connected) even when
  // the connection has failed.  this is easily demonstrated on OS X
  // 10.3.4 by starting a deskflow client and telling to connect to
  // another system that's not running a deskflow server.  it will
  // claim to have connected then quickly disconnect (i guess because
  // read returns 0 bytes).  unfortunately, deskflow attempts to
  // reconnect immediately, the process repeats and we end up
  // spinning the CPU.  luckily, OS X does set SO_ERROR on the
  // socket correctly when the connection has failed so checking for
  // errors works.  (curiously, sometimes OS X doesn't report
  // connection refused.  when that happens it at least doesn't
  // report the socket as being writable so deskflow is able to time
  // out the attempt.)
  if (error || true) {
    try {
      // connection may have failed or succeeded
      ARCH->throwErrorOnSocket(m_socket);
    } catch (const XArchNetwork &e) {
      sendConnectionFailedEvent(e.what());
      onDisconnected();
      return newJob();
    }
  }

  if (write) {
    sendEvent(m_events->forIDataSocket().connected());
    onConnected();
    return newJob();
  }

  return job;
}

ISocketMultiplexerJob *TCPSocket::serviceConnected(ISocketMultiplexerJob *job, bool read, bool write, bool error)
{
  Lock lock(&m_mutex);

  if (error) {
    sendEvent(m_events->forISocket().disconnected());
    onDisconnected();
    return newJob();
  }

  EJobResult result = kRetry;
  if (write) {
    try {
      result = doWrite();
    } catch (XArchNetworkShutdown &) {
      // remote read end of stream hungup.  our output side
      // has therefore shutdown.
      onOutputShutdown();
      sendEvent(m_events->forIStream().outputShutdown());
      if (!m_readable && m_inputBuffer.getSize() == 0) {
        sendEvent(m_events->forISocket().disconnected());
        m_connected = false;
      }
      result = kNew;
    } catch (XArchNetworkDisconnected &) {
      // stream hungup
      onDisconnected();
      sendEvent(m_events->forISocket().disconnected());
      result = kNew;
    } catch (XArchNetwork &e) {
      // other write error
      LOG((CLOG_WARN "error writing socket: %s", e.what()));
      onDisconnected();
      sendEvent(m_events->forIStream().outputError());
      sendEvent(m_events->forISocket().disconnected());
      result = kNew;
    }
  }

  if (read && m_readable) {
    try {
      result = doRead();
    } catch (XArchNetworkDisconnected &) {
      // stream hungup
      sendEvent(m_events->forISocket().disconnected());
      onDisconnected();
      result = kNew;
    } catch (XArchNetwork &e) {
      // ignore other read error
      LOG((CLOG_WARN "error reading socket: %s", e.what()));
    }
  }

  if (result == kBreak) {
    return nullptr;
  }

  return result == kNew ? newJob() : job;
}
