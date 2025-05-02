/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "EventTypes.h"
#include "common/Common.h"

using deskflow::EventTypes;

class EventData
{
public:
  EventData() = default;
  virtual ~EventData() = default;
};

//! Event
/*!
A \c Event holds an event type and a pointer to event data.
*/
class Event
{
public:
  using Flags = uint32_t;
  enum
  {
    kNone = 0x00,               //!< No flags
    kDeliverImmediately = 0x01, //!< Dispatch and free event immediately
    kDontFreeData = 0x02        //!< Don't free data in deleteData
  };

  Event();

  //! Create \c Event with data (POD)
  /*!
  The \p data must be POD (plain old data) allocated by malloc(),
  which means it cannot have a constructor, destructor or be
  composed of any types that do. For non-POD (normal C++ objects
  use \c setDataObject() or use appropriate constructor.
  \p target is the intended recipient of the event.
  \p flags is any combination of \c Flags.
  */
  Event(EventTypes type, void *target = nullptr, void *data = nullptr, Flags flags = kNone);

  //! Create \c Event with non-POD data
  /*!
  \p type of the event
  \p target is the intended recipient of the event.
  \p dataObject with event data
  */
  Event(EventTypes type, void *target, EventData *dataObject);

  //! @name manipulators
  //@{

  //! Release event data
  /*!
  Deletes event data for the given event (using free()).
  */
  static void deleteData(const Event &);

  //! Set data (non-POD)
  /*!
  Set non-POD (non plain old data), where delete is called when the event
  is deleted, and the destructor is called.
  */
  void setDataObject(EventData *dataObject);

  //@}
  //! @name accessors
  //@{

  //! Get event type
  /*!
  Returns the event type.
  */
  EventTypes getType() const;

  //! Get the event target
  /*!
  Returns the event target.
  */
  void *getTarget() const;

  //! Get the event data (POD).
  /*!
  Returns the event data (POD).
  */
  void *getData() const;

  //! Get the event data (non-POD)
  /*!
  Returns the event data (non-POD). The difference between this and
  \c getData() is that when delete is called on this data, so non-POD
  (non plain old data) dtor is called.
  */
  EventData *getDataObject() const;

  //! Get event flags
  /*!
  Returns the event flags.
  */
  Flags getFlags() const;

  //@}

private:
  EventTypes m_type;
  void *m_target;
  void *m_data;
  Flags m_flags;
  EventData *m_dataObject;
};
