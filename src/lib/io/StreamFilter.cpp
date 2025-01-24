/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "io/StreamFilter.h"
#include "base/IEventQueue.h"
#include "base/TMethodEventJob.h"

//
// StreamFilter
//

StreamFilter::StreamFilter(IEventQueue *events, deskflow::IStream *stream, bool adoptStream)
    : m_stream(stream),
      m_adopted(adoptStream),
      m_events(events)
{
  // replace handlers for m_stream
  m_events->removeHandlers(m_stream->getEventTarget());
  m_events->adoptHandler(
      Event::kUnknown, m_stream->getEventTarget(),
      new TMethodEventJob<StreamFilter>(this, &StreamFilter::handleUpstreamEvent)
  );
}

StreamFilter::~StreamFilter()
{
  m_events->removeHandler(Event::kUnknown, m_stream->getEventTarget());
  if (m_adopted) {
    delete m_stream;
  }
}

void StreamFilter::close()
{
  getStream()->close();
}

uint32_t StreamFilter::read(void *buffer, uint32_t n)
{
  return getStream()->read(buffer, n);
}

void StreamFilter::write(const void *buffer, uint32_t n)
{
  getStream()->write(buffer, n);
}

void StreamFilter::flush()
{
  getStream()->flush();
}

void StreamFilter::shutdownInput()
{
  getStream()->shutdownInput();
}

void StreamFilter::shutdownOutput()
{
  getStream()->shutdownOutput();
}

void *StreamFilter::getEventTarget() const
{
  return const_cast<void *>(static_cast<const void *>(this));
}

bool StreamFilter::isReady() const
{
  return getStream()->isReady();
}

uint32_t StreamFilter::getSize() const
{
  return getStream()->getSize();
}

deskflow::IStream *StreamFilter::getStream() const
{
  return m_stream;
}

void StreamFilter::filterEvent(const Event &event)
{
  m_events->dispatchEvent(Event(event.getType(), getEventTarget(), event.getData()));
}

void StreamFilter::handleUpstreamEvent(const Event &event, void *)
{
  filterEvent(event);
}
