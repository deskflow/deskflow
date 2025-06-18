/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IEventQueue.h"
#include "io/IStream.h"

//! A stream filter
/*!
This class wraps a stream.  Subclasses provide indirect access
to the wrapped stream, typically performing some filtering.
*/
class StreamFilter : public deskflow::IStream
{
public:
  /*!
  Create a wrapper around \c stream.  Iff \c adoptStream is true then
  this object takes ownership of the stream and will delete it in the
  d'tor.
  */
  StreamFilter(IEventQueue *events, deskflow::IStream *stream, bool adoptStream = true);
  StreamFilter(StreamFilter const &) = delete;
  StreamFilter(StreamFilter &&) = delete;
  ~StreamFilter() override;

  StreamFilter &operator=(StreamFilter const &) = delete;
  StreamFilter &operator=(StreamFilter &&) = delete;

  // IStream overrides
  // These all just forward to the underlying stream except getEventTarget.
  // Override as necessary.  getEventTarget returns a pointer to this.
  void close() override;
  uint32_t read(void *buffer, uint32_t n) override;
  void write(const void *buffer, uint32_t n) override;
  void flush() override;
  void shutdownInput() override;
  void shutdownOutput() override;
  void *getEventTarget() const override;
  bool isReady() const override;
  uint32_t getSize() const override;

  //! Get the stream
  /*!
  Returns the stream passed to the c'tor.
  */
  deskflow::IStream *getStream() const;

protected:
  //! Handle events from source stream
  /*!
  Does the event filtering.  The default simply dispatches an event
  identical except using this object as the event target.
  */
  virtual void filterEvent(const Event &);

private:
  deskflow::IStream *m_stream;
  bool m_adopted;
  IEventQueue *m_events;
};
