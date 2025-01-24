/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/PacketStreamFilter.h"
#include "base/IEventQueue.h"
#include "base/TMethodEventJob.h"
#include "deskflow/protocol_types.h"
#include "mt/Lock.h"

#include <cstring>
#include <memory>

//
// PacketStreamFilter
//

PacketStreamFilter::PacketStreamFilter(IEventQueue *events, deskflow::IStream *stream, bool adoptStream)
    : StreamFilter(events, stream, adoptStream),
      m_size(0),
      m_inputShutdown(false),
      m_events(events)
{
  // do nothing
}

PacketStreamFilter::~PacketStreamFilter()
{
  // do nothing
}

void PacketStreamFilter::close()
{
  Lock lock(&m_mutex);
  m_size = 0;
  m_buffer.pop(m_buffer.getSize());
  StreamFilter::close();
}

uint32_t PacketStreamFilter::read(void *buffer, uint32_t n)
{
  if (n == 0) {
    return 0;
  }

  Lock lock(&m_mutex);

  // if not enough data yet then give up
  if (!isReadyNoLock()) {
    return 0;
  }

  // read no more than what's left in the buffered packet
  if (n > m_size) {
    n = m_size;
  }

  // read it
  if (buffer != NULL) {
    memcpy(buffer, m_buffer.peek(n), n);
  }
  m_buffer.pop(n);
  m_size -= n;

  // get next packet's size if we've finished with this packet and
  // there's enough data to do so.
  readPacketSize();

  if (m_inputShutdown && m_size == 0) {
    m_events->addEvent(Event(m_events->forIStream().inputShutdown(), getEventTarget()));
  }

  return n;
}

void PacketStreamFilter::write(const void *buffer, uint32_t count)
{
  // write the length of the payload
  uint8_t length[4];
  length[0] = (uint8_t)((count >> 24) & 0xff);
  length[1] = (uint8_t)((count >> 16) & 0xff);
  length[2] = (uint8_t)((count >> 8) & 0xff);
  length[3] = (uint8_t)(count & 0xff);
  getStream()->write(length, sizeof(length));

  // write the payload
  getStream()->write(buffer, count);
}

void PacketStreamFilter::shutdownInput()
{
  Lock lock(&m_mutex);
  m_size = 0;
  m_buffer.pop(m_buffer.getSize());
  StreamFilter::shutdownInput();
}

bool PacketStreamFilter::isReady() const
{
  Lock lock(&m_mutex);
  return isReadyNoLock();
}

uint32_t PacketStreamFilter::getSize() const
{
  Lock lock(&m_mutex);
  return isReadyNoLock() ? m_size : 0;
}

bool PacketStreamFilter::isReadyNoLock() const
{
  return (m_size != 0 && m_buffer.getSize() >= m_size);
}

bool PacketStreamFilter::readPacketSize()
{
  // note -- m_mutex must be locked on entry

  if (m_size == 0 && m_buffer.getSize() >= 4) {
    uint8_t buffer[4];
    memcpy(buffer, m_buffer.peek(sizeof(buffer)), sizeof(buffer));
    m_buffer.pop(sizeof(buffer));
    m_size =
        ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) | ((uint32_t)buffer[2] << 8) | (uint32_t)buffer[3];
    if (m_size > PROTOCOL_MAX_MESSAGE_LENGTH) {
      m_events->addEvent(Event(m_events->forIStream().inputFormatError(), getEventTarget()));
      return false;
    }
  }
  return true;
}

bool PacketStreamFilter::readMore()
{
  // note if we have whole packet
  bool wasReady = isReadyNoLock();

  // read more data
  char buffer[4096];
  uint32_t n = getStream()->read(buffer, sizeof(buffer));
  while (n > 0) {
    m_buffer.write(buffer, n);

    // if we don't yet have the next packet size then get it, if possible.
    // Note that we can't wait for whole pending data to arrive because it may be huge in
    // case of malicious or erroneous peer.
    if (!readPacketSize()) {
      break;
    }

    n = getStream()->read(buffer, sizeof(buffer));
  }

  // note if we now have a whole packet
  bool isReady = isReadyNoLock();

  // if we weren't ready before but now we are then send a
  // input ready event apparently from the filtered stream.
  return (wasReady != isReady);
}

void PacketStreamFilter::filterEvent(const Event &event)
{
  if (event.getType() == m_events->forIStream().inputReady()) {
    Lock lock(&m_mutex);
    if (!readMore()) {
      return;
    }
  } else if (event.getType() == m_events->forIStream().inputShutdown()) {
    // discard this if we have buffered data
    Lock lock(&m_mutex);
    m_inputShutdown = true;
    if (m_size != 0) {
      return;
    }
  }

  // pass event
  StreamFilter::filterEvent(event);
}
