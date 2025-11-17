/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiClipboard.h"

#include "base/Log.h"
#include "deskflow/ClipboardTypes.h"

namespace deskflow {

EiClipboard::EiClipboard()
{
  initialize();
}

EiClipboard::~EiClipboard()
{
  cleanup();
}

bool EiClipboard::isAvailable() const
{
  return m_available;
}

IClipboard *EiClipboard::getClipboard(ClipboardID id) const
{
  if (!m_available || id >= m_clipboards.size()) {
    return nullptr;
  }

  return m_clipboards[id].get();
}

bool EiClipboard::hasChanged() const
{
  if (!m_available) {
    return false;
  }

  for (const auto &clipboard : m_clipboards) {
    if (clipboard && clipboard->hasChanged()) {
      return true;
    }
  }

  return false;
}

void EiClipboard::startMonitoring()
{
  if (!m_available || m_monitoring) {
    return;
  }

  for (auto &clipboard : m_clipboards) {
    if (clipboard) {
      clipboard->startMonitoring();
    }
  }

  m_monitoring = true;
}

void EiClipboard::stopMonitoring()
{
  if (!m_available || !m_monitoring) {
    return;
  }

  for (auto &clipboard : m_clipboards) {
    if (clipboard) {
      clipboard->stopMonitoring();
    }
  }

  m_monitoring = false;
}

void EiClipboard::resetChanged()
{
  if (!m_available) {
    return;
  }

  for (auto &clipboard : m_clipboards) {
    if (clipboard) {
      clipboard->resetChanged();
    }
  }
}

void EiClipboard::initialize()
{
  // Check if Wayland clipboard is available
  if (!WlClipboard::isAvailable()) {
    LOG_WARN("wl-clipboard tools not found, clipboard functionality disabled");
    return;
  }

  // Create clipboard instances for each clipboard type
  m_clipboards.resize(kClipboardEnd);

  try {
    // Primary clipboard (selection)
    m_clipboards[kClipboardSelection] = std::make_unique<WlClipboard>(kClipboardSelection);

    // Standard clipboard
    m_clipboards[kClipboardClipboard] = std::make_unique<WlClipboard>(kClipboardClipboard);

    m_available = true;
    LOG_DEBUG1("initialized Wayland clipboard support");

  } catch (const std::exception &e) {
    LOG_ERR("failed to initialize clipboard: %s", e.what());
    cleanup();
    m_available = false;
  }
}

void EiClipboard::cleanup()
{
  if (m_monitoring) {
    stopMonitoring();
  }

  m_clipboards.clear();
  m_available = false;
}

} // namespace deskflow
