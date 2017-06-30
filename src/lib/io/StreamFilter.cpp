/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "io/StreamFilter.h"
#include "base/IEventQueue.h"
#include "base/TMethodEventJob.h"

//
// StreamFilter
//

StreamFilter::StreamFilter (IEventQueue* events, synergy::IStream* stream,
                            bool adoptStream)
    : m_stream (stream), m_adopted (adoptStream), m_events (events) {
    // replace handlers for m_stream
    m_events->removeHandlers (m_stream->getEventTarget ());
    m_events->adoptHandler (Event::kUnknown,
                            m_stream->getEventTarget (),
                            new TMethodEventJob<StreamFilter> (
                                this, &StreamFilter::handleUpstreamEvent));
}

StreamFilter::~StreamFilter () {
    m_events->removeHandler (Event::kUnknown, m_stream->getEventTarget ());
    if (m_adopted) {
        delete m_stream;
    }
}

void
StreamFilter::close () {
    getStream ()->close ();
}

UInt32
StreamFilter::read (void* buffer, UInt32 n) {
    return getStream ()->read (buffer, n);
}

void
StreamFilter::write (const void* buffer, UInt32 n) {
    getStream ()->write (buffer, n);
}

void
StreamFilter::flush () {
    getStream ()->flush ();
}

void
StreamFilter::shutdownInput () {
    getStream ()->shutdownInput ();
}

void
StreamFilter::shutdownOutput () {
    getStream ()->shutdownOutput ();
}

void*
StreamFilter::getEventTarget () const {
    return const_cast<void*> (static_cast<const void*> (this));
}

bool
StreamFilter::isReady () const {
    return getStream ()->isReady ();
}

UInt32
StreamFilter::getSize () const {
    return getStream ()->getSize ();
}

synergy::IStream*
StreamFilter::getStream () const {
    return m_stream;
}

void
StreamFilter::filterEvent (const Event& event) {
    m_events->dispatchEvent (
        Event (event.getType (), getEventTarget (), event.getData ()));
}

void
StreamFilter::handleUpstreamEvent (const Event& event, void*) {
    filterEvent (event);
}
