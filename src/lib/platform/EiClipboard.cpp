/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Applone <app1oneme@gmail.com>
 * SPDX-FileCopyrightText: (C) 2026 Red Hat, Inc.
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

uint64_t EiClipboard::revision() const
{
  std::scoped_lock lock{m_mutex};
  return m_revision;
}

bool EiClipboard::replace(const std::vector<std::pair<Format, std::string>> &values, uint64_t expectedRevision)
{
  std::scoped_lock lock{m_mutex};
  if (m_revision != expectedRevision)
    return false;
  ++m_revision;
  for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
    m_added[i] = false;
    m_data[i].clear();
  }
  for (const auto &[format, data] : values) {
    const auto i = static_cast<int>(format);
    m_added[i] = true;
    m_data[i] = data;
  }
  m_owner = true;
  m_timeOwned = m_time;
  return true;
}

bool EiClipboard::empty()
{
  std::scoped_lock lock{m_mutex};
  if (!m_open) {
    LOG_WARN("cannot empty clipboard, not open");
    return false;
  }

  ++m_revision;
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
  std::scoped_lock lock{m_mutex};
  if (!m_open) {
    LOG_WARN("cannot add to clipboard, not open");
    return;
  }

  if (!m_owner) {
    LOG_WARN("cannot add to clipboard, no owner");
    return;
  }

  ++m_revision;
  const auto formatID = static_cast<int>(format);
  m_data[formatID] = data;
  m_added[formatID] = true;
}

bool EiClipboard::open(Time time) const
{
  std::scoped_lock lock{m_mutex};
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
  std::scoped_lock lock{m_mutex};
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
  std::scoped_lock lock{m_mutex};
  if (!m_open) {
    LOG_WARN("cannot check for clipboard format, not open");
    return false;
  }
  return m_added[static_cast<int>(format)];
}

std::string EiClipboard::get(Format format) const
{
  std::scoped_lock lock{m_mutex};
  if (!m_open) {
    LOG_WARN("cannot get clipboard format, not open");
    return "";
  }
  return m_data[static_cast<int>(format)];
}

} // namespace deskflow
