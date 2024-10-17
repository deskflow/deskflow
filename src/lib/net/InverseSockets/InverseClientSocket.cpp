/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2022 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "InverseClientSocket.h"

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

//
// InverseClientSocket
//

InverseClientSocket::InverseClientSocket(
    IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family
)
    : IDataSocket(events),
      m_events(events),
      m_socket(family),
      m_listener(family),
      m_flushed(&m_mutex, true),
      m_socketMultiplexer(socketMultiplexer)
{
}

InverseClientSocket::~InverseClientSocket()
{
  try {
    // warning virtual function in destructor is very danger practice
    InverseClientSocket::close();
  } catch (...) {
    LOG((CLOG_DEBUG "error while TCP socket destruction"));
  }
}

void InverseClientSocket::bind(const NetworkAddress &addr)
{
  m_socket.bindSocket(addr);
}

void InverseClientSocket::close()
{
  setJob(nullptr);

  Lock lock(&m_mutex);
  onDisconnected();
}

void *InverseClientSocket::getEventTarget() const
{
  return const_cast<void *>(static_cast<const void *>(this));
}

UInt32 InverseClientSocket::read(void *buffer, UInt32 n)
{
  // copy data directly from our input buffer
  Lock lock(&m_mutex);
  UInt32 size = m_inputBuffer.getSize();
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

void InverseClientSocket::write(const void *buffer, UInt32 n)
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
    setJob(newJob(m_socket.getRawSocket()));
  }
}

void InverseClientSocket::flush()
{
  Lock lock(&m_mutex);
  while (m_flushed == false) {
    m_flushed.wait();
  }
}

void InverseClientSocket::shutdownInput()
{
  bool useNewJob = false;
  {
    Lock lock(&m_mutex);

    // shutdown socket for reading
    m_socket.closeSocketForRead();

    // shutdown buffer for reading
    if (m_readable) {
      sendEvent(m_events->forIStream().inputShutdown());
      onInputShutdown();
      useNewJob = true;
    }
  }
  if (useNewJob) {
    setJob(newJob(m_socket.getRawSocket()));
  }
}

void InverseClientSocket::shutdownOutput()
{
  bool useNewJob = false;
  {
    Lock lock(&m_mutex);

    // shutdown socket for writing
    m_socket.closeSocketForWrite();

    // shutdown buffer for writing
    if (m_writable) {
      sendEvent(m_events->forIStream().outputShutdown());
      onOutputShutdown();
      useNewJob = true;
    }
  }
  if (useNewJob) {
    setJob(newJob(m_socket.getRawSocket()));
  }
}

bool InverseClientSocket::isReady() const
{
  Lock lock(&m_mutex);
  return (m_inputBuffer.getSize() > 0);
}

bool InverseClientSocket::isFatal() const
{
  // TCP sockets aren't ever left in a fatal state.
  LOG((CLOG_ERR "isFatal() not valid for non-secure connections"));
  return false;
}

UInt32 InverseClientSocket::getSize() const
{
  Lock lock(&m_mutex);
  return m_inputBuffer.getSize();
}

void InverseClientSocket::connect(const NetworkAddress &addr)
{
  {
    Lock lock(&m_mutex);
    m_listener.bindAndListen(addr);
    m_writable = true;
    m_readable = true;
  }
  setJob(newJob(m_listener.getRawSocket()));
}

InverseClientSocket::EJobResult InverseClientSocket::doRead()
{
  UInt8 buffer[4096] = {0};
  size_t bytesRead = m_socket.readSocket(buffer, sizeof(buffer));

  if (bytesRead > 0) {
    bool wasEmpty = (m_inputBuffer.getSize() == 0);

    // slurp up as much as possible
    do {
      m_inputBuffer.write(buffer, static_cast<UInt32>(bytesRead));

      bytesRead = m_socket.readSocket(buffer, sizeof(buffer));
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
    return InverseClientSocket::EJobResult::kNew;
  }

  return InverseClientSocket::EJobResult::kRetry;
}

InverseClientSocket::EJobResult InverseClientSocket::doWrite()
{
  UInt32 bufferSize = m_outputBuffer.getSize();
  auto buffer = static_cast<const UInt8 *>(m_outputBuffer.peek(bufferSize));
  const auto bytesWrote = static_cast<UInt32>(m_socket.writeSocket(buffer, bufferSize));

  if (bytesWrote > 0) {
    discardWrittenData(bytesWrote);
    return InverseClientSocket::EJobResult::kNew;
  }

  return InverseClientSocket::EJobResult::kRetry;
}

void InverseClientSocket::setJob(ISocketMultiplexerJob *job)
{
  // multiplexer will delete the old job
  if (job == nullptr) {
    m_socketMultiplexer->removeSocket(this);
  } else {
    m_socketMultiplexer->addSocket(this, job);
  }
}

ISocketMultiplexerJob *InverseClientSocket::newJob(ArchSocket socket)
{
  // note -- must have m_mutex locked on entry

  ISocketMultiplexerJob *result = nullptr;

  if (socket) {
    auto isWritable = m_writable;
    auto handler = &InverseClientSocket::serviceConnecting;

    if (m_connected) {
      handler = &InverseClientSocket::serviceConnected;
      isWritable = (isWritable && (m_outputBuffer.getSize() > 0));
    }

    if (m_readable || isWritable) {
      result = new TSocketMultiplexerMethodJob<InverseClientSocket>(this, handler, socket, m_readable, isWritable);
    }
  }

  return result;
}

void InverseClientSocket::sendConnectionFailedEvent(const char *msg)
{
  auto info = new ConnectionFailedInfo(msg);
  m_events->addEvent(Event(m_events->forIDataSocket().connectionFailed(), getEventTarget(), info, Event::kDontFreeData)
  );
}

void InverseClientSocket::sendEvent(Event::Type type)
{
  m_events->addEvent(Event(type, getEventTarget()));
}

void InverseClientSocket::discardWrittenData(int bytesWrote)
{
  m_outputBuffer.pop(bytesWrote);
  if (m_outputBuffer.getSize() == 0) {
    sendEvent(m_events->forIStream().outputFlushed());
    m_flushed = true;
    m_flushed.broadcast();
  }
}

void InverseClientSocket::onConnected()
{
  sendEvent(m_events->forIDataSocket().connected());
  m_connected = true;
  m_readable = true;
  m_writable = true;
}

void InverseClientSocket::onInputShutdown()
{
  m_inputBuffer.pop(m_inputBuffer.getSize());
  m_readable = false;
}

void InverseClientSocket::onOutputShutdown()
{
  m_outputBuffer.pop(m_outputBuffer.getSize());
  m_writable = false;

  // we're now flushed
  m_flushed = true;
  m_flushed.broadcast();
}

void InverseClientSocket::onDisconnected()
{
  if (m_connected) {
    sendEvent(m_events->forISocket().disconnected());
  }
  // disconnected
  onInputShutdown();
  onOutputShutdown();
  m_connected = false;
}

ISocketMultiplexerJob *InverseClientSocket::serviceConnecting(ISocketMultiplexerJob *job, bool read, bool, bool)
{
  Lock lock(&m_mutex);

  if (read) {
    m_socket = m_listener.acceptSocket();
    onConnected();
    return newJob(m_socket.getRawSocket());
  }

  return job;
}

ISocketMultiplexerJob *
InverseClientSocket::serviceConnected(ISocketMultiplexerJob *job, bool read, bool write, bool error)
{
  Lock lock(&m_mutex);

  if (error) {
    onDisconnected();
    return newJob(m_listener.getRawSocket());
  }

  EJobResult result = InverseClientSocket::EJobResult::kRetry;
  if (write) {
    try {
      result = doWrite();
    } catch (const XArchNetworkShutdown &) {
      // remote read end of stream hungup.  our output side
      // has therefore shutdown.
      onOutputShutdown();
      sendEvent(m_events->forIStream().outputShutdown());
      if (!m_readable && m_inputBuffer.getSize() == 0) {
        sendEvent(m_events->forISocket().disconnected());
        m_connected = false;
      }
      result = InverseClientSocket::EJobResult::kNew;
    } catch (const XArchNetworkDisconnected &) {
      // stream hungup
      onDisconnected();
      result = InverseClientSocket::EJobResult::kNew;
    } catch (const XArchNetwork &e) {
      // other write error
      LOG((CLOG_WARN "error writing socket: %s", e.what()));
      onDisconnected();
      sendEvent(m_events->forIStream().outputError());
      result = InverseClientSocket::EJobResult::kNew;
    }
  }

  if (read && m_readable) {
    try {
      result = doRead();
    } catch (const XArchNetworkDisconnected &) {
      // stream hungup
      onDisconnected();
      result = InverseClientSocket::EJobResult::kNew;
    } catch (const XArchNetwork &e) {
      // ignore other read error
      LOG((CLOG_WARN "error reading socket: %s", e.what()));
    }
  }

  if (result == InverseClientSocket::EJobResult::kBreak) {
    return nullptr;
  }

  return result == InverseClientSocket::EJobResult::kNew ? newJob(m_socket.getRawSocket()) : job;
}
