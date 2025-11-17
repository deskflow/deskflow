/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
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
class EiClipboard
{
public:
  EiClipboard();
  ~EiClipboard();

  //! Check if clipboard functionality is available
  bool isAvailable() const;

  //! Get clipboard for specific ID
  IClipboard *getClipboard(ClipboardID id) const;

  //! Check if any clipboard has changed
  bool hasChanged() const;

  //! Start monitoring clipboard changes
  void startMonitoring();

  //! Stop monitoring clipboard changes
  void stopMonitoring();

  //! Reset change detection
  void resetChanged();

private:
  //! Initialize clipboard backends
  void initialize();

  //! Cleanup clipboard backends
  void cleanup();

private:
  std::vector<std::unique_ptr<WlClipboard>> m_clipboards;
  bool m_available = false;
  bool m_monitoring = false;
};

} // namespace deskflow
