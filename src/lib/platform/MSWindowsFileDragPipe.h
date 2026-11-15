/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

/**
 * @file MSWindowsFileDragPipe.h
 * @brief The link between deskflow-core and its file drag helper (deskflow-filedrag.exe)
 *
 * Windows does not let OLE drag and drop cross integrity levels, and deskflow-core often
 * runs elevated (as SYSTEM in the user's session). Everything that touches OLE therefore
 * runs in a small helper process that is started with the signed-in user's own token.
 * The core and the helper talk over two anonymous pipes that only the helper inherits.
 *
 * A frame is: u32 length (big endian, counts the kind byte and the payload), u8 kind,
 * payload. Payloads are written with deskflow::filedrag::ByteWriter.
 *
 * This file only depends on Windows and the C++ library so the helper and the test
 * harness can build it without the rest of Deskflow.
 */

namespace deskflow::filedrag::pipe {

enum class Frame : uint8_t
{
  // core -> helper
  WatchEdges = 1, ///< u32 sides (DirectionMask), i32 x, y, w, h of the screen
  HandOff = 2,    ///< u32 token, u32 dragId, i32 x, y of the parked cursor; answered by SyncAck
  TakeBack = 3,   ///< u32 token; answered by SyncAck
  Peer = 4,       ///< encodeMessage() bytes, a message from / for the network peer (both directions)
  Shutdown = 5,   ///< no payload
  Entered = 6,    ///< the cursor is back on this (primary) computer: drop anything still parked

  // helper -> core
  Hello = 32,         ///< u32 capability flags, u32 helper process id
  OfferReady = 33,    ///< a file drag waits at a watched edge
  OfferLost = 34,     ///< it left the edge again
  LauncherReady = 35, ///< u32 dragId, i32 x, i32 y: press the left button there to start the drag
  LoopEnded = 36,     ///< u32 dragId, u8 dropped
  SyncAck = 37,       ///< u32 token
  Log = 38,           ///< u8 level (0 error, 1 warning, 2 info, 3 debug), text

  // test harness only (helper started with --test)
  TestSourceEnter = 64, ///< u32 count, bytes16 UTF-8 paths: a shell drag of these paths enters the edge strip
  TestSourceLeave = 65, ///< the test drag leaves the strip again
  TestDropTarget = 66,  ///< bytes16 UTF-8 folder: the next drag loop runs against this folder, not DoDragDrop
  TestRelease = 67,     ///< u8 escape: the simulated button goes up (0) or escape is pressed (1)
  TestEvent = 96,       ///< helper -> harness: text
};

const char *frameName(Frame frame);

//! Largest frame accepted (a chunk message is ~32 KB, an item list at most 1 MB)
constexpr uint32_t kMaxFrameBytes = 4u * 1024 * 1024;

//! Writes frames to a pipe; safe to use from several threads
class FrameWriter
{
public:
  explicit FrameWriter(HANDLE pipe = nullptr) : m_pipe(pipe)
  {
  }
  void setPipe(HANDLE pipe)
  {
    m_pipe = pipe;
  }
  bool write(Frame frame, std::string_view payload = {});

private:
  HANDLE m_pipe;
  std::mutex m_mutex;
};

//! Reads one frame; false when the pipe broke or the frame is malformed
bool readFrame(HANDLE pipe, Frame &frame, std::string &payload);

//! A started helper process and the core's ends of its pipes
struct HelperProcess
{
  HANDLE process = nullptr;
  HANDLE job = nullptr;
  HANDLE toHelper = nullptr;
  HANDLE fromHelper = nullptr;
  DWORD pid = 0;

  void close();
};

/**
 * @brief Starts \p exe with pipes on the command line ("--pipe-in N --pipe-out N")
 *
 * With a \p userToken the helper runs as that user (CreateProcessAsUserW, needs SYSTEM),
 * otherwise as the caller. \p desktop (e.g. L"winsta0\\default") may be null. Only the two
 * pipe ends are inherited, no window is shown, and the helper is put in a job that ends
 * it when the caller exits.
 */
bool startHelper(
    const std::wstring &exe, const std::wstring &extraArgs, HANDLE userToken, const wchar_t *desktop,
    HelperProcess &out, std::string &error
);

//! UTF-8 <-> UTF-16
std::wstring widen(std::string_view utf8);
std::string narrow(std::wstring_view wide);

//! Text of a Windows error code
std::string errorText(DWORD code);

} // namespace deskflow::filedrag::pipe
