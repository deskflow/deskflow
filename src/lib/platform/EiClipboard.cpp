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
  std::scoped_lock lock(m_mutex);

  if (!m_open) {
    LOG_WARN("clipboard not open");
    return false;
  }

  for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
    m_data[i].clear();
    m_added[i] = false;
  }

  m_timeOwned = m_time;
  m_owner = true;

  return true;
}

void EiClipboard::add(Format format, const std::string &data)
{
  std::scoped_lock lock(m_mutex);

  if (!m_open) {
    LOG_WARN("clipboard not open");
    return;
  }

  if (!m_owner) {
    LOG_WARN("cannot add to clipboard without ownership");
    return;
  }

  const auto idx = static_cast<int>(format);
  m_data[idx] = data;
  m_added[idx] = true;
}

bool EiClipboard::open(Time time) const
{
  std::scoped_lock lock(m_mutex);

  if (m_open) {
    return true;
  }

  m_open = true;
  m_time = time;

  return true;
}

void EiClipboard::close() const
{
  std::scoped_lock lock(m_mutex);
  m_open = false;
}

IClipboard::Time EiClipboard::getTime() const
{
  std::scoped_lock lock(m_mutex);
  return m_timeOwned;
}

bool EiClipboard::has(Format format) const
{
  std::scoped_lock lock(m_mutex);

  if (!m_open) {
    return false;
  }

  return m_added[static_cast<int>(format)];
}

std::string EiClipboard::get(Format format) const
{
  std::scoped_lock lock(m_mutex);

  if (!m_open) {
    return {};
  }

  return m_data[static_cast<int>(format)];
}

} // namespace deskflow
