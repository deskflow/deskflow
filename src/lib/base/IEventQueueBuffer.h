/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <assert.h>
#include <cstdint>

class Event;
class EventQueueTimer;

//! Event queue buffer interface
/*!
An event queue buffer provides a queue of events for an IEventQueue.
*/
class IEventQueueBuffer
{
public:
  virtual ~IEventQueueBuffer() = default;
  enum class Type : uint8_t
  {
    Unknown, //!< No event is available
    System,  //!< Event is a system event
    User     //!< Event is a user event
  };

  //! @name manipulators
  //@{

  //! Initialize
  /*!
  Useful for platform-specific initialisation from a specific thread.
  */
  virtual void init() = 0;

  //! Block waiting for an event
  /*!
  Wait for an event in the event queue buffer for up to \p timeout
  seconds.
  */
  virtual void waitForEvent(double timeout) = 0;

  //! Get the next event
  /*!
  Get the next event from the buffer.  Return None if no event is
  available.  If a system event is next, return System and fill in
  event.  The event data in a system event can point to a static
  buffer (because Event::deleteData() will not attempt to delete
  data in a System event).  Otherwise, return User and fill in
  \p dataID with the value passed to \c addEvent().
  */
  virtual Type getEvent(Event &event, uint32_t &dataID) = 0;

  //! Post an event
  /*!
  Add the given event to the end of the queue buffer.  This is a user
  event and \c getEvent() must be able to identify it as such and
  return \p dataID.  This method must cause \c waitForEvent() to
  return at some future time if it's blocked waiting on an event.
  */
  virtual bool addEvent(uint32_t dataID) = 0;

  //! Check if event queue buffer is empty
  /*!
  Return true iff the event queue buffer  is empty.
  */
  virtual bool isEmpty() const = 0;
};
