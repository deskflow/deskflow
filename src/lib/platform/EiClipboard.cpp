/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiClipboard.h"
#include "base/Log.h"

namespace deskflow {

EiClipboard::EiClipboard(ClipboardID id) : m_id(id)
{
  open(0);
  empty();
  close();
}

bool EiClipboard::empty()
{
  if (!m_open) {
    LOG_WARN("cannot empty clipboard, not open");
    return false;
  }

  // Clear all data
  for (int32_t index = 0; index < static_cast<int>(Format::TotalFormats); ++index) {
    m_data[index] = "";
    m_added[index] = false;
  }

  // Save time
  m_timeOwned = m_time;

  // We're the owner now
  m_owner = true;

  return true;
}

void EiClipboard::add(Format format, const std::string &data)
{
  if (!m_open) {
    LOG_WARN("cannot add to clipboard, not open");
    return;
  }

  if (!m_owner) {
    LOG_WARN("cannot add to clipboard, no owner");
    return;
  }

  const auto formatID = static_cast<int>(format);
  m_data[formatID] = data;
  m_added[formatID] = true;
}

bool EiClipboard::open(Time time) const
{
  if (m_open) {
    LOG_DEBUG("skipping clipboard open, already open");
    return true;
  }

  m_open = true;
  m_time = time;

  return true;
}

void EiClipboard::close() const
{
  if (!m_open) {
    LOG_WARN("clipboard is not open");
  }
  m_open = false;
}

EiClipboard::Time EiClipboard::getTime() const
{
  return m_timeOwned;
}

bool EiClipboard::has(Format format) const
{
  if (!m_open) {
    LOG_WARN("cannot check for clipboard format, not open");
    return false;
  }
  return m_added[static_cast<int>(format)];
}

std::string EiClipboard::get(Format format) const
{
  if (!m_open) {
    LOG_WARN("cannot get clipboard format, not open");
    return "";
  }
  return m_data[static_cast<int>(format)];
}

} // namespace deskflow
