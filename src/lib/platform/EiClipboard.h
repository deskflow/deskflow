/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"
#include "deskflow/IClipboard.h"

#include <mutex>

namespace deskflow {

//! In-memory clipboard for EI/portal clipboard integration
/*!
  Stores clipboard data locally so the portal can serve it on demand
  via SelectionTransfer, and so we can track what the remote has offered
  via SelectionOwnerChanged.
*/
class EiClipboard : public IClipboard
{
public:
  explicit EiClipboard(ClipboardID id);
  ~EiClipboard() override = default;

  ClipboardID getID() const
  {
    return m_id;
  }

  // IClipboard overrides
  bool empty() override;
  void add(Format format, const std::string &data) override;
  bool open(Time time) const override;
  void close() const override;
  Time getTime() const override;
  bool has(Format format) const override;
  std::string get(Format format) const override;

private:
  ClipboardID m_id;
  mutable bool m_open = false;
  mutable Time m_time = 0;
  bool m_owner = false;
  Time m_timeOwned = 0;
  bool m_added[static_cast<int>(Format::TotalFormats)] = {};
  std::string m_data[static_cast<int>(Format::TotalFormats)];

  // Protects concurrent access from the main thread (setClipboard) and
  // the GLib event thread (portal signal handlers).
  mutable std::mutex m_mutex;
};

} // namespace deskflow
