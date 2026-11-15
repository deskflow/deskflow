/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/FileDragTypes.h"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace deskflow::filedrag {

//! Appends big-endian integers and length-prefixed byte strings
class ByteWriter
{
public:
  void u8(uint8_t v);
  void u16(uint16_t v);
  void u32(uint32_t v);
  void u64(uint64_t v);
  //! 2 byte length then bytes (at most 65535)
  void bytes16(std::string_view v);
  //! 4 byte length then bytes
  void bytes32(std::string_view v);

  const std::string &str() const
  {
    return m_out;
  }
  std::string take()
  {
    return std::move(m_out);
  }

private:
  std::string m_out;
};

//! Reads what ByteWriter wrote; every read fails (and stays failed) past the end
class ByteReader
{
public:
  explicit ByteReader(std::string_view in) : m_in(in)
  {
  }

  bool u8(uint8_t &v);
  bool u16(uint16_t &v);
  bool u32(uint32_t &v);
  bool u64(uint64_t &v);
  bool bytes16(std::string &v);
  bool bytes32(std::string &v, uint32_t maxLength);

  bool ok() const
  {
    return m_ok;
  }
  bool atEnd() const
  {
    return m_pos == m_in.size();
  }

private:
  bool take(size_t n, const char *&p);

  std::string_view m_in;
  size_t m_pos = 0;
  bool m_ok = true;
};

//! Encodes the item list carried by kMsgDFileDragBegin
std::string encodeItems(const ItemList &items);

//! Decodes and validates (validateItems) an item list; \p error says why it failed
bool decodeItems(std::string_view encoded, ItemList &items, std::string *error = nullptr);

/**
 * @brief Checks an item list before anything is written to disk
 *
 * Rejects lists with too many items, unsafe paths (see isSafeRelativePath), folders
 * with a size, duplicate paths, or items whose parent folder is not listed before them.
 */
bool validateItems(const ItemList &items, std::string *error = nullptr);

/**
 * @brief True if \p path is a safe relative drag path
 *
 * The path must be relative ('/' separated), non-empty, at most kMaxPathBytes long, and
 * must not be absolute ("/x", "\\x", "C:x", "//server"), contain empty, "." or ".."
 * components (with '/' or '\' as separator), or control characters.
 */
bool isSafeRelativePath(std::string_view path);

//! Splits a '/' separated path into components (empty components are kept)
std::vector<std::string> splitPath(std::string_view path);

/**
 * @brief Makes one path component safe for the local file system
 *
 * Characters Windows forbids (\ / : * ? " < > |) become '_', trailing dots and spaces
 * are removed, reserved device names (CON, NUL, COM1, ...) get a '_' prefix, and a
 * component that ends up empty, "." or ".." becomes "_". The same rules are used on all
 * platforms so a drag lands with the same names everywhere.
 */
std::string sanitizeComponent(std::string_view component);

//! Sanitizes every component of a safe relative path
std::string sanitizeRelativePath(std::string_view path);

/**
 * @brief Returns \p name, or the first "name (N).ext" that \p taken says is free
 *
 * The extension (after the last dot that is not the first character) is kept for
 * files; folders never split an extension. Never returns a taken name.
 */
std::string
uniqueName(const std::string &name, bool isDirectory, const std::function<bool(const std::string &)> &taken);

//! Case-insensitive (ASCII and Latin letters via simple folding) key for name collisions
std::string nameKey(std::string_view name);

//! Sum of all item sizes; false on overflow
bool totalSize(const ItemList &items, uint64_t &total);

//! Encodes a message for a local transport (e.g. the Windows helper pipe)
std::string encodeMessage(const Message &message);

//! Decodes encodeMessage() output; validates item lists and icons
bool decodeMessage(std::string_view encoded, Message &message, std::string *error = nullptr);

/**
 * @brief Turns premultiplied RGBA8 pixels into straight alpha, as Icon carries them
 *
 * Window systems hand out premultiplied pixels (colour already scaled by alpha). Fully
 * transparent pixels become 0,0,0,0. Trailing bytes that do not make a pixel are left alone.
 */
void unpremultiplyRgba(std::string &rgba);

} // namespace deskflow::filedrag
