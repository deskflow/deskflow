/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/Clipboard.h"
#include "base/Log.h"

//
// Clipboard
//

Clipboard::Clipboard() : m_open(false), m_owner(false)
{
  open(0);
  empty();
  close();
}

bool Clipboard::empty()
{
  if (!m_open) {
    LOG_WARN("cannot empty clipboard, not open");
    return false;
  }

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
  if (!m_open) {
    LOG_WARN("cannot add to clipboard, not open");
    return;
  }

  if (!m_owner) {
    LOG_WARN("cannot add to clipboard, no owner");
    return;
  }

  m_data[format] = data;
  m_added[format] = true;
}

bool Clipboard::open(Time time) const
{
  if (m_open) {
    LOG_DEBUG("skipping clipboard open, already open");
    return true;
  }

  m_open = true;
  m_time = time;

  return true;
}

void Clipboard::close() const
{
  if (!m_open) {
    LOG_WARN("clipboard is not open");
  }
  m_open = false;
}

Clipboard::Time Clipboard::getTime() const
{
  return m_timeOwned;
}

bool Clipboard::has(EFormat format) const
{
  if (!m_open) {
    LOG_WARN("cannot check for clipboard format, not open");
    return false;
  }
  return m_added[format];
}

std::string Clipboard::get(EFormat format) const
{
  if (!m_open) {
    LOG_WARN("cannot get clipboard format, not open");
    return "";
  }
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
