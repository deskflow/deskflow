/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"
#include "deskflow/IClipboard.h"

#include <mutex>
#include <optional>
#include <string>

#include <QByteArray>
#include <QString>
#include <QStringList>

//! Wayland clipboard implementation using wl-copy/wl-paste
/*!
This class implements clipboard functionality for Wayland environments
by using the wl-clipboard utilities (wl-copy and wl-paste).

Data added between open() and close() is accumulated and committed as a
single wl-copy invocation on close(), since wl-copy can only offer one
mime type per invocation.

Change detection is on demand via hasChanged() rather than a polling
thread: on compositors without a data-control protocol (e.g. GNOME)
wl-paste briefly steals keyboard focus, so it must only run at moments
where that cannot interrupt typing, such as when leaving the screen.
*/
class WlClipboard : public IClipboard
{
public:
  explicit WlClipboard(ClipboardID id);
  WlClipboard(WlClipboard const &) = delete;
  WlClipboard(WlClipboard &&) = delete;
  ~WlClipboard() override = default;

  WlClipboard &operator=(WlClipboard const &) = delete;
  WlClipboard &operator=(WlClipboard &&) = delete;

  //! Get clipboard ID
  ClipboardID getID() const;

  //! Check if wl-clipboard tools are available
  static bool isAvailable();

  //! Check if WlClipboard is enabled
  static bool isEnabled();

  //! Check the clipboard for an external change since the last call.
  //! Spawns wl-paste, so only call when a focus blip cannot interrupt
  //! the user (e.g. when leaving the screen). The first call only
  //! records a baseline.
  bool hasChanged() const;

  //! Clear cached clipboard data so the next read is fresh
  void resetChanged();

  // IClipboard overrides
  bool empty() override;
  void add(Format format, const std::string &data) override;
  bool open(Time time) const override;
  void close() const override;
  Time getTime() const override;
  bool has(Format format) const override;
  std::string get(Format format) const override;

private:
  //! Convert IClipboard format to MIME type
  QString formatToMimeType(Format format) const;

  //! Get available MIME types from clipboard
  QStringList getAvailableMimeTypes() const;

  //! Run wl-paste with the given args, bounded by maxBytes (-1 = unlimited) and timeoutMs
  QByteArray readClipboard(const QStringList &args, qsizetype maxBytes, int timeoutMs) const;

  //! Write the accumulated data as a single wl-copy invocation
  void commitPendingData() const;

  //! Get current clipboard serial/timestamp
  Time getCurrentTime() const;

  //! Invalidate cached clipboard data
  void invalidateCache() const;

private:
  ClipboardID m_id;
  mutable bool m_open = false;
  mutable Time m_time = 0;

  // Data accumulated between empty()/add() and the commit on close()
  mutable bool m_pendingWrite = false;
  mutable std::string m_pendingData[static_cast<int>(Format::TotalFormats)];
  mutable bool m_pendingAvailable[static_cast<int>(Format::TotalFormats)] = {};

  // Cached clipboard data
  mutable std::mutex m_cacheMutex;
  mutable bool m_cached = false;
  mutable Time m_cachedTime = 0;
  mutable std::string m_cachedData[static_cast<int>(Format::TotalFormats)];
  mutable bool m_cachedAvailable[static_cast<int>(Format::TotalFormats)] = {};

  // Fingerprint of the clipboard as of the last hasChanged() call
  mutable bool m_haveFingerprint = false;
  mutable QStringList m_lastTypes;
  mutable size_t m_lastContentHash = 0;

  // Content hash of our own last commit, used to not report it as a change
  mutable std::optional<size_t> m_ownWriteHash;

  // Clipboard selection type (true = clipboard, false = primary)
  bool m_useClipboard;
};
