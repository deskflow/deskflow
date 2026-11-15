/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

/**
 * @file FileDragTypes.h
 * @brief Platform neutral types for dragging files and folders between computers.
 *
 * A file drag starts when the user drags files with the mouse across a switch edge. The
 * computer the files come from (the source) sends the item list (kMsgDFileDragBegin) and
 * streams the file contents in small chunks while the other computer (the destination)
 * shows the drag under its cursor. The destination stages the data in a temporary folder
 * and hands it to the drop target when the user lets go. See ProtocolTypes.h for the wire
 * format.
 *
 * Both directions use the same messages: server to client (the server is the source) and
 * client to server (the client is the source; the server tells it which edge to watch with
 * kMsgDFileDragWatch, hears about files there with kMsgDFileDragOffer and takes them over
 * with kMsgDFileDragHandOff).
 *
 * Nothing in here touches a window system; the platform layers (Windows, and later macOS
 * and X11) only translate between these types and their native drag-and-drop APIs.
 */

namespace deskflow::filedrag {

//! Capability flags exchanged with kMsgDFileDragCaps
enum Capability : uint32_t
{
  kCapNone = 0,
  kCapSource = 1u << 0,      ///< Can notice a file drag leaving this computer and send its files
  kCapDestination = 1u << 1, ///< Can start a file drag on this computer and receive the files
  /// Understands the client to server messages (kMsgDFileDragWatch, kMsgDFileDragOffer,
  /// kMsgDFileDragHandOff). Peers built before them never set it, so they never get them.
  kCapReverse = 1u << 2,
};

//! True if the server (with \p serverCaps) may take file drags from a client with \p clientCaps
constexpr bool reverseDragAvailable(uint32_t serverCaps, uint32_t clientCaps)
{
  const uint32_t server = kCapDestination | kCapReverse;
  const uint32_t client = kCapSource | kCapReverse;
  return (serverCaps & server) == server && (clientCaps & client) == client;
}

//! Why a drag was cancelled, carried by kMsgDFileDragCancel
enum class CancelReason : uint8_t
{
  Unknown = 0,
  User = 1,           ///< Right click or the drop target refused the items
  Escape = 2,         ///< Escape pressed during the drag
  CursorReturned = 3, ///< Cursor went back to the primary before the drop
  ConnectionLost = 4, ///< Peer disconnected or stopped answering
  NoSpace = 5,        ///< Not enough free disk space to stage the items
  ReadFailed = 6,     ///< A source file could not be read (or changed while sending)
  WriteFailed = 7,    ///< A staged file could not be written
  Invalid = 8,        ///< Item list or message rejected (bad path, size mismatch, ...)
  DropFailed = 9,     ///< The drop target failed to take the items
  Timeout = 10,       ///< A step took too long
  Busy = 11,          ///< Another drag is still running
  Unsupported = 12,   ///< This computer cannot show a drag right now
};

const char *cancelReasonName(CancelReason reason);
CancelReason cancelReasonFromInt(uint32_t value);

//! Size of the file chunks on the wire. Small so input events never wait long.
constexpr uint32_t kChunkSize = 32 * 1024;
//! Largest chunk a receiver accepts.
constexpr uint32_t kMaxChunkSize = 64 * 1024;
//! Most items (files and folders) in one drag.
constexpr uint32_t kMaxItems = 20000;
//! Longest relative path in bytes (UTF-8).
constexpr uint32_t kMaxPathBytes = 1024;
//! Longest single path component in bytes (UTF-8).
constexpr uint32_t kMaxComponentBytes = 255;
//! Largest drag icon side in pixels.
constexpr uint16_t kMaxIconSide = 256;
//! Largest encoded item list (must fit a protocol string).
constexpr uint32_t kMaxItemListBytes = 1024 * 1024;
//! Free space to keep on the staging disk on top of the drag itself.
constexpr uint64_t kStagingSpareBytes = 64ull * 1024 * 1024;

//! One file or folder of a drag
struct Item
{
  std::string path;  ///< Relative to the drag, '/' separated UTF-8, e.g. "Photos/2026/a.jpg"
  uint64_t size = 0; ///< Bytes; always 0 for folders
  bool isDirectory = false;

  bool operator==(const Item &) const = default;
};

using ItemList = std::vector<Item>;

//! Drag image: RGBA8, straight alpha, top row first
struct Icon
{
  uint16_t width = 0;
  uint16_t height = 0;
  std::string rgba;

  bool isEmpty() const
  {
    return width == 0 || height == 0;
  }
  bool isValid() const;
  bool operator==(const Icon &) const = default;
};

//! One file drag protocol message, independent of the transport
struct Message
{
  enum class Type : uint8_t
  {
    Caps = 1,
    Icon = 2,
    Begin = 3,
    FileStart = 4,
    Chunk = 5,
    FileEnd = 6,
    Ack = 7,
    Cancel = 8,
    Dropped = 9,
    Finished = 10,
    Watch = 11,   ///< server -> client: watch these sides (flags, a DirectionMask; 0 stops)
    Offer = 12,   ///< client -> server: files are at a watched edge (flags 1) or gone (flags 0)
    HandOff = 13, ///< server -> client: the cursor took the files to the server as dragId
  };

  Type type = Type::Caps;
  uint32_t dragId = 0;
  uint32_t index = 0; ///< Item index (FileStart, Chunk, FileEnd)
  uint32_t seq = 0;   ///< Chunk sequence (Chunk) or highest written chunk (Ack)
  uint32_t flags = 0; ///< Capability flags (Caps), sides (Watch), 1 = ready (Offer)
  uint64_t size = 0;  ///< File size (FileStart)
  CancelReason reason = CancelReason::Unknown;
  Icon icon;
  ItemList items;
  std::string data; ///< Chunk bytes

  bool operator==(const Message &) const = default;

  static Message caps(uint32_t flags);
  static Message cancel(uint32_t dragId, CancelReason reason);
  static Message ack(uint32_t dragId, uint32_t seq);
  static Message dropped(uint32_t dragId);
  static Message finished(uint32_t dragId);
  static Message watch(uint32_t sides);
  static Message offer(bool ready);
  static Message handOff(uint32_t dragId);
};

const char *messageTypeName(Message::Type type);

} // namespace deskflow::filedrag
