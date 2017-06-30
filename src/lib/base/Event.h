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

#pragma once

#include "common/basic_types.h"
#include "common/stdmap.h"

class EventData {
public:
    EventData () {
    }
    virtual ~EventData () {
    }
};

//! Event
/*!
A \c Event holds an event type and a pointer to event data.
*/
class Event {
public:
    typedef UInt32 Type;
    enum {
        kUnknown, //!< The event type is unknown
        kQuit,    //!< The quit event
        kSystem,  //!< The data points to a system event type
        kTimer,   //!< The data points to timer info
        kLast     //!< Must be last
    };

    typedef UInt32 Flags;
    enum {
        kNone               = 0x00, //!< No flags
        kDeliverImmediately = 0x01, //!< Dispatch and free event immediately
        kDontFreeData       = 0x02  //!< Don't free data in deleteData
    };

    Event ();

    //! Create \c Event with data (POD)
    /*!
    The \p data must be POD (plain old data) allocated by malloc(),
    which means it cannot have a constructor, destructor or be
    composed of any types that do. For non-POD (normal C++ objects
    use \c setDataObject().
    \p target is the intended recipient of the event.
    \p flags is any combination of \c Flags.
    */
    Event (Type type, void* target = NULL, void* data = NULL,
           Flags flags = kNone);

    //! @name manipulators
    //@{

    //! Release event data
    /*!
    Deletes event data for the given event (using free()).
    */
    static void deleteData (const Event&);

    //! Set data (non-POD)
    /*!
    Set non-POD (non plain old data), where delete is called when the event
    is deleted, and the destructor is called.
    */
    void setDataObject (EventData* dataObject);

    //@}
    //! @name accessors
    //@{

    //! Get event type
    /*!
    Returns the event type.
    */
    Type getType () const;

    //! Get the event target
    /*!
    Returns the event target.
    */
    void* getTarget () const;

    //! Get the event data (POD).
    /*!
    Returns the event data (POD).
    */
    void* getData () const;

    //! Get the event data (non-POD)
    /*!
    Returns the event data (non-POD). The difference between this and
    \c getData() is that when delete is called on this data, so non-POD
    (non plain old data) dtor is called.
    */
    EventData* getDataObject () const;

    //! Get event flags
    /*!
    Returns the event flags.
    */
    Flags getFlags () const;

    //@}

private:
    Type m_type;
    void* m_target;
    void* m_data;
    Flags m_flags;
    EventData* m_dataObject;
};
