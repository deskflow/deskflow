/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"
#include "deskflow/IClipboard.h"
#include "platform/WlClipboard.h"

#if defined(WINAPI_LIBPORTAL) && WINAPI_LIBPORTAL
#include "platform/PortalClipboard.h"
#endif

#include <memory>
#include <vector>

namespace deskflow {

//! Clipboard backend type enumeration
enum class ClipboardBackend
{
  Portal, //!< XDG Desktop Portal (preferred for Wayland)
  WlClipboard //!< wl-copy/wl-paste tools (fallback)
};

//! Clipboard manager for EiScreen
/*!
This class manages clipboard operations for the EiScreen implementation.
Supports multiple backends:
1. XDG Desktop Portal (preferred) - for sandboxed Wayland environments
2. wl-clipboard tools (fallback) - requires wl-copy/wl-paste

Backend selection:
- Portal is tried first when available and enabled
- Falls back to wl-clipboard if portal is unavailable

See: https://github.com/deskflow/deskflow/issues/8031
*/
class WlClipboardCollection
{
public:
  WlClipboardCollection();
  ~WlClipboardCollection();

  //! Check if clipboard functionality is available
  bool isAvailable() const;

  //! Get the active backend type
  ClipboardBackend getActiveBackend() const;

  //! Get clipboard for specific ID
  IClipboard *getClipboard(ClipboardID id) const;

  //! Check if any clipboard has changed
  bool hasChanged() const;

  //! Start monitoring clipboard changes
  void startMonitoring();

  //! Stop monitoring clipboard changes
  void stopMonitoring();

  //! Reset change detection
  void resetChanged() const;

private:
  //! Initialize clipboard backends
  void initialize();

  //! Try to initialize portal clipboard backend
  bool tryInitializePortal();

  //! Initialize wl-clipboard backend
  bool initializeWlClipboard();

  //! Cleanup clipboard backends
  void cleanup();

private:
  std::vector<std::unique_ptr<IClipboard>> m_clipboards;
  ClipboardBackend m_backend = ClipboardBackend::WlClipboard;
  bool m_available = false;
  bool m_monitoring = false;
};

} // namespace deskflow
