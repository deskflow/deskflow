/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/Clipboard.h"

//
// Clipboard
//

Clipboard::Clipboard() : m_open(false), m_owner(false)
{
  open(0);
  empty();
  close();
}

Clipboard::~Clipboard()
{
  // do nothing
}

bool Clipboard::empty()
{
  assert(m_open);

  // clear all data
  for (int32_t index = 0; index < kNumFormats; ++index) {
    m_data[index] = "";
    m_added[index] = false;
  }

  // save time
  m_timeOwned = m_time;

  // we're the owner now
  m_owner = true;

  return true;
}

void Clipboard::add(EFormat format, const std::string &data)
{
  assert(m_open);
  assert(m_owner);

  m_data[format] = data;
  m_added[format] = true;
}

bool Clipboard::open(Time time) const
{
  assert(!m_open);

  m_open = true;
  m_time = time;

  return true;
}

void Clipboard::close() const
{
  assert(m_open);

  m_open = false;
}

Clipboard::Time Clipboard::getTime() const
{
  return m_timeOwned;
}

bool Clipboard::has(EFormat format) const
{
  assert(m_open);
  return m_added[format];
}

std::string Clipboard::get(EFormat format) const
{
  assert(m_open);
  return m_data[format];
}

void Clipboard::unmarshall(const std::string &data, Time time)
{
  IClipboard::unmarshall(this, data, time);
}

std::string Clipboard::marshall() const
{
  return IClipboard::marshall(this);
}
