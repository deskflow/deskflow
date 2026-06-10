/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"
#include "deskflow/IClipboard.h"
#include "platform/WlClipboard.h"

#include <memory>
#include <vector>

namespace deskflow {

//! Clipboard manager for EiScreen
/*!
This class manages clipboard operations for the EiScreen implementation.
It automatically detects the best available clipboard backend and provides
a unified interface for clipboard operations.
*/
class WlClipboardCollection
{
public:
  WlClipboardCollection();
  ~WlClipboardCollection();

  //! Check if clipboard functionality is available
  bool isAvailable() const;

  //! Get clipboard for specific ID
  IClipboard *getClipboard(ClipboardID id) const;

  //! Check the given clipboard for an external change since the last call.
  //! Spawns wl-paste, so only call when a focus blip cannot interrupt the
  //! user (e.g. when leaving the screen).
  bool hasChanged(ClipboardID id) const;

  //! Reset change detection for the given clipboard
  void resetChanged(ClipboardID id) const;

private:
  //! Initialize clipboard backends
  void initialize();

  //! Cleanup clipboard backends
  void cleanup();

private:
  std::vector<std::unique_ptr<WlClipboard>> m_clipboards;
  bool m_available = false;
};

} // namespace deskflow
