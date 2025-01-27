/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/IInterface.h"
#include "common/common.h"

class Event;
class EventQueueTimer;

//! Event queue buffer interface
/*!
An event queue buffer provides a queue of events for an IEventQueue.
*/
class IEventQueueBuffer : public IInterface
{
public:
  enum Type
  {
    kNone,   //!< No event is available
    kSystem, //!< Event is a system event
    kUser    //!< Event is a user event
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
  Get the next event from the buffer.  Return kNone if no event is
  available.  If a system event is next, return kSystem and fill in
  event.  The event data in a system event can point to a static
  buffer (because Event::deleteData() will not attempt to delete
  data in a kSystem event).  Otherwise, return kUser and fill in
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

  //@}
  //! @name accessors
  //@{

  //! Check if event queue buffer is empty
  /*!
  Return true iff the event queue buffer  is empty.
  */
  virtual bool isEmpty() const = 0;

  //! Create a timer object
  /*!
  Create and return a timer object.  The object is opaque and is
  used only by the buffer but it must be a valid object (i.e.
  not NULL).
  */
  virtual EventQueueTimer *newTimer(double duration, bool oneShot) const = 0;

  //! Destroy a timer object
  /*!
  Destroy a timer object previously returned by \c newTimer().
  */
  virtual void deleteTimer(EventQueueTimer *) const = 0;

  //@}
};
