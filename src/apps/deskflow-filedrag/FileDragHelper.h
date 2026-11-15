/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/FileDragTypes.h"
#include "platform/MSWindowsFileDragPipe.h"

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file FileDragHelper.h
 * @brief deskflow-filedrag.exe: the OLE side of dragging files between computers
 *
 * deskflow-core starts this helper with the signed-in user's token, because OLE drag and
 * drop does not work between programs of different integrity levels and the core often
 * runs elevated. The helper owns every window and COM object of a file drag:
 *
 * - source role: thin edge windows that notice a file drag arriving at a switch edge, a
 *   "catcher" window that parks the local drag while the cursor is on the other computer,
 *   and the sender that streams the files;
 * - destination role: the staging folder, a launcher window that starts the drag under the
 *   cursor, and the data object Explorer reads the files from.
 *
 * Both roles run on both computers: the server is the source of server to client drags
 * and the destination of client to server drags, and the client the other way round.
 * --primary / --secondary only say which computer this is (for the log).
 *
 * Threads: a UI thread (single-threaded apartment, windows, OLE) and a pipe reader thread
 * (multi-threaded apartment). File data is written on the reader thread so it never waits
 * for the UI.
 */

namespace deskflow::filedrag::helper {

enum class LogLevel : uint8_t
{
  Error = 0,
  Warning = 1,
  Info = 2,
  Debug = 3,
};

struct Options
{
  bool primary = true; ///< started by the server (--primary) or by a client (--secondary)
  bool test = false;
  std::filesystem::path stagingRoot; ///< empty: FileDragStaging::defaultRoot()
};

const Options &options();

//! Logs through the core (and the debugger)
void log(LogLevel level, const char *format, ...);

//! Sends a frame to the core
void sendFrame(pipe::Frame frame, std::string_view payload = {});
//! Sends a message to the network peer (through the core)
void sendPeer(const Message &message);
//! Test harness only: a status line
void testEvent(const std::string &text);

//! Runs \p task on the UI thread
void postToUi(std::function<void()> task);
//! The UI thread's hidden control window
HWND controlWindow();
//! The module instance
HINSTANCE instance();

//! Creates a hidden, topmost, never activated window that still takes mouse input
HWND createOverlayWindow(const wchar_t *className, WNDPROC proc, void *self);

// source role
void sourceWatchEdges(uint32_t sides, const RECT &screen);
void sourceHandOff(uint32_t dragId, POINT parked);
void sourceTakeBack();
void sourceEntered();
void sourcePeer(const Message &message); ///< reader thread
void sourceTestEnter(const std::vector<std::wstring> &paths);
void sourceTestLeave();
void sourceShutdown();

// destination role
void destStart();
void destPeer(const Message &message); ///< reader thread
void destTestDropTarget(const std::wstring &folder);
void destTestRelease(bool escape); ///< any thread
void destShutdown();
void destRetryRemoval(); ///< control window timer 7

} // namespace deskflow::filedrag::helper
