/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Event.h"

//
// Event
//

Event::Event() : m_type(kUnknown), m_target(NULL), m_data(NULL), m_flags(0), m_dataObject(nullptr)
{
  // do nothing
}

Event::Event(Type type, void *target, void *data, Flags flags)
    : m_type(type),
      m_target(target),
      m_data(data),
      m_flags(flags),
      m_dataObject(nullptr)
{
  // do nothing
}

Event::Event(Type type, void *target, EventData *dataObject)
    : m_type(type),
      m_target(target),
      m_data(nullptr),
      m_flags(kNone),
      m_dataObject(dataObject)
{
}

Event::Type Event::getType() const
{
  return m_type;
}

void *Event::getTarget() const
{
  return m_target;
}

void *Event::getData() const
{
  return m_data;
}

EventData *Event::getDataObject() const
{
  return m_dataObject;
}

Event::Flags Event::getFlags() const
{
  return m_flags;
}

void Event::deleteData(const Event &event)
{
  switch (event.getType()) {
  case kUnknown:
  case kQuit:
  case kSystem:
  case kTimer:
    break;

  default:
    if ((event.getFlags() & kDontFreeData) == 0) {
      free(event.getData());
      delete event.getDataObject();
    }
    break;
  }
}

void Event::setDataObject(EventData *dataObject)
{
  assert(m_dataObject == nullptr);
  m_dataObject = dataObject;
}
