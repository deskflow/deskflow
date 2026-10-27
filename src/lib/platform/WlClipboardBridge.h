/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IClipboard.h"

#include <QString>
#include <QStringList>

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace deskflow {

//! Commands used to list, read, watch and write the clipboard (overridable for tests)
/*!
The mime type is appended as the last argument to \c paste and \c copy.
*/
struct WlClipboardCommands
{
  QStringList list = {QStringLiteral("wl-paste"), QStringLiteral("--list-types")};
  QStringList paste = {QStringLiteral("wl-paste"), QStringLiteral("--no-newline"), QStringLiteral("--type")};
  QStringList watch = {
      QStringLiteral("wl-paste"), QStringLiteral("--watch"), QStringLiteral("sh"), QStringLiteral("-c"),
      QStringLiteral("cat >/dev/null; echo")
  };
  QStringList copy = {QStringLiteral("wl-copy"), QStringLiteral("--type")};
};

//! Wayland clipboard bridge based on wl-clipboard
/*!
Some compositors (e.g. Hyprland) do not implement the XDG Clipboard portal, so
the portal-based clipboard never receives any data. This bridge watches the
local clipboard with `wl-paste --watch` and writes remote clipboard data with
`wl-copy`, which only requires the wlr data-control protocol.

wl-copy can offer a single mime type, so only one format is written at a time.
*/
class WlClipboardBridge
{
public:
  //! Clipboard content keyed by mime type
  using MimeData = std::map<std::string, std::string>;
  using ChangedCallback = std::function<void(const MimeData &data)>;
  using Commands = WlClipboardCommands;

  static constexpr auto kMimeText = "text/plain;charset=utf-8";
  static constexpr auto kMimeHtml = "text/html";
  static constexpr auto kMimePng = "image/png";

  explicit WlClipboardBridge(ChangedCallback onChanged, Commands commands = {});
  ~WlClipboardBridge();

  WlClipboardBridge(const WlClipboardBridge &) = delete;
  WlClipboardBridge &operator=(const WlClipboardBridge &) = delete;

  //! True when running on Wayland, wl-clipboard is installed and the bridge is not disabled
  /*!
  Set DESKFLOW_WL_CLIPBOARD=0 to disable, or =1 to force it on desktops that
  already provide the Clipboard portal (GNOME, KDE).
  */
  static bool isAvailable();

  //! Start watching the local clipboard
  void start();

  //! Stop watching the local clipboard
  void stop();

  //! Replace the local clipboard (data received from a remote computer)
  void setData(const std::string &mime, const std::string &data);

  //! Read the supported formats (text, html, png) of the local clipboard
  MimeData readData() const;

  //! Fill an open clipboard with the formats in \p data, returns false if none was usable
  static bool toClipboard(const MimeData &data, IClipboard *clipboard);

  //! Pick the single format to write locally from an open clipboard: image, else text, else html
  /*!
  Text wins over html because wl-copy would also expose html as plain text,
  pasting raw markup into terminals and plain text fields.
  */
  static std::pair<std::string, std::string> fromClipboard(const IClipboard *clipboard);

private:
  QStringList listTypes() const;
  std::string readMime(const QString &mime) const;
  bool isOwnWrite(const MimeData &data) const;
  void watchLoop();
  void handleChange();

  ChangedCallback m_onChanged;
  Commands m_commands;
  std::thread m_thread;
  std::atomic_bool m_stopping = false;
  mutable std::mutex m_mutex;
  MimeData m_lastRead;
  std::string m_lastWrittenMime;
  std::string m_lastWrittenData;
};

} // namespace deskflow
