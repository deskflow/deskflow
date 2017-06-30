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

#include "base/Event.h"
#include "base/EventQueue.h"

//
// Event
//

Event::Event ()
    : m_type (kUnknown),
      m_target (NULL),
      m_data (NULL),
      m_flags (0),
      m_dataObject (nullptr) {
    // do nothing
}

Event::Event (Type type, void* target, void* data, Flags flags)
    : m_type (type),
      m_target (target),
      m_data (data),
      m_flags (flags),
      m_dataObject (nullptr) {
    // do nothing
}

Event::Type
Event::getType () const {
    return m_type;
}

void*
Event::getTarget () const {
    return m_target;
}

void*
Event::getData () const {
    return m_data;
}

EventData*
Event::getDataObject () const {
    return m_dataObject;
}

Event::Flags
Event::getFlags () const {
    return m_flags;
}

void
Event::deleteData (const Event& event) {
    switch (event.getType ()) {
        case kUnknown:
        case kQuit:
        case kSystem:
        case kTimer:
            break;

        default:
            if ((event.getFlags () & kDontFreeData) == 0) {
                free (event.getData ());
                delete event.getDataObject ();
            }
            break;
    }
}

void
Event::setDataObject (EventData* dataObject) {
    assert (m_dataObject == nullptr);
    m_dataObject = dataObject;
}
