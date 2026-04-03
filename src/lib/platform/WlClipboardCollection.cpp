/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/WlClipboardCollection.h"

#include "base/Log.h"
#include "deskflow/ClipboardTypes.h"

// Note: PortalClipboard is conditionally compiled when libportal with clipboard support is available
// For now, we only use wl-clipboard backend. Portal clipboard support will be enabled when
// xdg-desktop-portal backends implement the clipboard portal.
// See: https://github.com/deskflow/deskflow/issues/8031

namespace deskflow {

WlClipboardCollection::WlClipboardCollection()
{
  initialize();
}

WlClipboardCollection::~WlClipboardCollection()
{
  cleanup();
}

bool WlClipboardCollection::isAvailable() const
{
  return m_available;
}

ClipboardBackend WlClipboardCollection::getActiveBackend() const
{
  return m_backend;
}

IClipboard *WlClipboardCollection::getClipboard(ClipboardID id) const
{
  if (!m_available || id >= m_clipboards.size()) {
    return nullptr;
  }

  return m_clipboards[id].get();
}

bool WlClipboardCollection::hasChanged() const
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

void WlClipboardCollection::startMonitoring()
{
  if (!m_available || m_monitoring) {
    return;
  }

  for (const auto &clipboard : m_clipboards) {
    if (clipboard) {
      clipboard->startMonitoring();
    }
  }

  m_monitoring = true;
}

void WlClipboardCollection::stopMonitoring()
{
  if (!m_available || !m_monitoring) {
    return;
  }

  for (const auto &clipboard : m_clipboards) {
    if (clipboard) {
      clipboard->stopMonitoring();
    }
  }

  m_monitoring = false;
}

void WlClipboardCollection::resetChanged() const
{
  if (!m_available) {
    return;
  }

  for (const auto &clipboard : m_clipboards) {
    if (clipboard) {
      clipboard->resetChanged();
    }
  }
}

void WlClipboardCollection::initialize()
{
  // First, check if clipboard is disabled in settings
  if (!WlClipboard::isEnabled()) {
    LOG_DEBUG("clipboard setting disabled");
    return;
  }

  // Try portal clipboard first (preferred for sandboxed environments)
  // Note: Portal clipboard is disabled for now until xdg-desktop-portal backends
  // implement the clipboard portal. When available, this will be tried first.
  // See: https://github.com/deskflow/deskflow/issues/8031
#if 0 // Portal clipboard disabled until backend support is available
  if (tryInitializePortal()) {
    m_backend = ClipboardBackend::Portal;
    m_available = true;
    LOG_INFO("using portal clipboard backend");
    return;
  }
#endif

  // Fall back to wl-clipboard tools
  if (initializeWlClipboard()) {
    m_backend = ClipboardBackend::WlClipboard;
    m_available = true;
    LOG_INFO("using wl-clipboard backend");
    return;
  }

  LOG_WARN("no clipboard backend available, clipboard functionality disabled");
}

bool WlClipboardCollection::tryInitializePortal()
{
  // Portal clipboard is not yet implemented
  // This will be enabled when xdg-desktop-portal backends implement clipboard support
  // See: https://github.com/deskflow/deskflow/issues/8031
  //
  // When implemented, this will:
  // 1. Check if PortalClipboard::isAvailable() returns true
  // 2. Create PortalClipboard instances for each clipboard type
  // 3. Return true if successful

  LOG_DEBUG("portal clipboard not yet available (waiting for xdg-desktop-portal support)");
  return false;
}

bool WlClipboardCollection::initializeWlClipboard()
{
  if (!WlClipboard::isAvailable()) {
    LOG_DEBUG("wl-clipboard tools not found");
    return false;
  }

  // Create clipboard instances for each clipboard type
  m_clipboards.resize(kClipboardEnd);

  try {
    // Primary clipboard (selection)
    m_clipboards[kClipboardSelection] = std::make_unique<WlClipboard>(kClipboardSelection);

    // Standard clipboard
    m_clipboards[kClipboardClipboard] = std::make_unique<WlClipboard>(kClipboardClipboard);

    LOG_DEBUG("initialized wl-clipboard backend");
    return true;

  } catch (const std::exception &e) {
    LOG_ERR("failed to initialize wl-clipboard: %s", e.what());
    cleanup();
    return false;
  }
}

void WlClipboardCollection::cleanup()
{
  if (m_monitoring) {
    stopMonitoring();
  }

  m_clipboards.clear();
  m_available = false;
}

} // namespace deskflow
