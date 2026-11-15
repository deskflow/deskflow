/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/FileDragCodec.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <set>
#include <unordered_set>

namespace deskflow::filedrag {

//
// names
//

const char *cancelReasonName(CancelReason reason)
{
  switch (reason) {
    using enum CancelReason;
  case User:
    return "cancelled by user";
  case Escape:
    return "escape pressed";
  case CursorReturned:
    return "cursor returned";
  case ConnectionLost:
    return "connection lost";
  case NoSpace:
    return "not enough disk space";
  case ReadFailed:
    return "source file could not be read";
  case WriteFailed:
    return "staged file could not be written";
  case Invalid:
    return "invalid drag data";
  case DropFailed:
    return "drop target failed";
  case Timeout:
    return "timed out";
  case Busy:
    return "another drag is running";
  case Unsupported:
    return "drag not supported here";
  case Unknown:
  default:
    return "unknown";
  }
}

CancelReason cancelReasonFromInt(uint32_t value)
{
  if (value >= 1 && value <= static_cast<uint32_t>(CancelReason::Unsupported)) {
    return static_cast<CancelReason>(value);
  }
  return CancelReason::Unknown;
}

const char *messageTypeName(Message::Type type)
{
  switch (type) {
    using enum Message::Type;
  case Caps:
    return "caps";
  case Icon:
    return "icon";
  case Begin:
    return "begin";
  case FileStart:
    return "file-start";
  case Chunk:
    return "chunk";
  case FileEnd:
    return "file-end";
  case Ack:
    return "ack";
  case Cancel:
    return "cancel";
  case Dropped:
    return "dropped";
  case Finished:
    return "finished";
  case Watch:
    return "watch";
  case Offer:
    return "offer";
  case HandOff:
    return "hand-off";
  default:
    return "?";
  }
}

bool Icon::isValid() const
{
  if (width == 0 || height == 0) {
    return rgba.empty() && width == 0 && height == 0;
  }
  return width <= kMaxIconSide && height <= kMaxIconSide &&
         rgba.size() == static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
}

Message Message::caps(uint32_t flags)
{
  Message m;
  m.type = Type::Caps;
  m.flags = flags;
  return m;
}

Message Message::cancel(uint32_t dragId, CancelReason reason)
{
  Message m;
  m.type = Type::Cancel;
  m.dragId = dragId;
  m.reason = reason;
  return m;
}

Message Message::ack(uint32_t dragId, uint32_t seq)
{
  Message m;
  m.type = Type::Ack;
  m.dragId = dragId;
  m.seq = seq;
  return m;
}

Message Message::dropped(uint32_t dragId)
{
  Message m;
  m.type = Type::Dropped;
  m.dragId = dragId;
  return m;
}

Message Message::finished(uint32_t dragId)
{
  Message m;
  m.type = Type::Finished;
  m.dragId = dragId;
  return m;
}

Message Message::watch(uint32_t sides)
{
  Message m;
  m.type = Type::Watch;
  m.flags = sides;
  return m;
}

Message Message::offer(bool ready)
{
  Message m;
  m.type = Type::Offer;
  m.flags = ready ? 1 : 0;
  return m;
}

Message Message::handOff(uint32_t dragId)
{
  Message m;
  m.type = Type::HandOff;
  m.dragId = dragId;
  return m;
}

//
// ByteWriter / ByteReader
//

void ByteWriter::u8(uint8_t v)
{
  m_out.push_back(static_cast<char>(v));
}

void ByteWriter::u16(uint16_t v)
{
  u8(static_cast<uint8_t>(v >> 8));
  u8(static_cast<uint8_t>(v));
}

void ByteWriter::u32(uint32_t v)
{
  u16(static_cast<uint16_t>(v >> 16));
  u16(static_cast<uint16_t>(v));
}

void ByteWriter::u64(uint64_t v)
{
  u32(static_cast<uint32_t>(v >> 32));
  u32(static_cast<uint32_t>(v));
}

void ByteWriter::bytes16(std::string_view v)
{
  const auto n = static_cast<uint16_t>(std::min<size_t>(v.size(), 0xffff));
  u16(n);
  m_out.append(v.data(), n);
}

void ByteWriter::bytes32(std::string_view v)
{
  u32(static_cast<uint32_t>(v.size()));
  m_out.append(v.data(), v.size());
}

bool ByteReader::take(size_t n, const char *&p)
{
  if (!m_ok || m_in.size() - m_pos < n) {
    m_ok = false;
    return false;
  }
  p = m_in.data() + m_pos;
  m_pos += n;
  return true;
}

bool ByteReader::u8(uint8_t &v)
{
  const char *p = nullptr;
  if (!take(1, p)) {
    return false;
  }
  v = static_cast<uint8_t>(p[0]);
  return true;
}

bool ByteReader::u16(uint16_t &v)
{
  uint8_t a = 0;
  uint8_t b = 0;
  if (!u8(a) || !u8(b)) {
    return false;
  }
  v = static_cast<uint16_t>((a << 8) | b);
  return true;
}

bool ByteReader::u32(uint32_t &v)
{
  uint16_t a = 0;
  uint16_t b = 0;
  if (!u16(a) || !u16(b)) {
    return false;
  }
  v = (static_cast<uint32_t>(a) << 16) | b;
  return true;
}

bool ByteReader::u64(uint64_t &v)
{
  uint32_t a = 0;
  uint32_t b = 0;
  if (!u32(a) || !u32(b)) {
    return false;
  }
  v = (static_cast<uint64_t>(a) << 32) | b;
  return true;
}

bool ByteReader::bytes16(std::string &v)
{
  uint16_t n = 0;
  const char *p = nullptr;
  if (!u16(n) || !take(n, p)) {
    return false;
  }
  v.assign(p, n);
  return true;
}

bool ByteReader::bytes32(std::string &v, uint32_t maxLength)
{
  uint32_t n = 0;
  if (!u32(n)) {
    return false;
  }
  if (n > maxLength) {
    m_ok = false;
    return false;
  }
  const char *p = nullptr;
  if (!take(n, p)) {
    return false;
  }
  v.assign(p, n);
  return true;
}

//
// paths
//

namespace {

void setError(std::string *error, const std::string &message)
{
  if (error != nullptr) {
    *error = message;
  }
}

bool isAsciiLetter(char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

std::string parentOf(std::string_view path)
{
  const auto slash = path.rfind('/');
  if (slash == std::string_view::npos) {
    return {};
  }
  return std::string(path.substr(0, slash));
}

bool isReservedDeviceName(std::string_view component)
{
  // the part before the first dot decides: "nul.txt" is as reserved as "NUL"
  const auto dot = component.find('.');
  std::string base(component.substr(0, dot));
  // trailing spaces are ignored by Windows too
  while (!base.empty() && base.back() == ' ') {
    base.pop_back();
  }
  std::transform(base.begin(), base.end(), base.begin(), [](unsigned char c) {
    return static_cast<char>(std::toupper(c));
  });
  static const std::array<const char *, 4> fixed = {"CON", "PRN", "AUX", "NUL"};
  for (const auto *name : fixed) {
    if (base == name) {
      return true;
    }
  }
  if (base.size() == 4 && (base.starts_with("COM") || base.starts_with("LPT"))) {
    const char d = base[3];
    return d >= '0' && d <= '9';
  }
  if (base == "CONIN$" || base == "CONOUT$") {
    return true;
  }
  return false;
}

} // namespace

std::vector<std::string> splitPath(std::string_view path)
{
  std::vector<std::string> parts;
  size_t start = 0;
  while (true) {
    const auto slash = path.find('/', start);
    if (slash == std::string_view::npos) {
      parts.emplace_back(path.substr(start));
      break;
    }
    parts.emplace_back(path.substr(start, slash - start));
    start = slash + 1;
  }
  return parts;
}

bool isSafeRelativePath(std::string_view path)
{
  if (path.empty() || path.size() > kMaxPathBytes) {
    return false;
  }
  for (const char c : path) {
    if (static_cast<unsigned char>(c) < 0x20 || c == 0x7f) {
      return false;
    }
  }
  // absolute or rooted: "/x", "\x", "//server", "C:", "C:\x"
  if (path.front() == '/' || path.front() == '\\') {
    return false;
  }
  if (path.size() >= 2 && isAsciiLetter(path[0]) && path[1] == ':') {
    return false;
  }

  // check components with both separators so "..\x" is caught too
  size_t start = 0;
  for (size_t i = 0; i <= path.size(); ++i) {
    if (i == path.size() || path[i] == '/' || path[i] == '\\') {
      const auto component = path.substr(start, i - start);
      if (component.empty() || component == "." || component == "..") {
        return false;
      }
      if (component.size() > kMaxComponentBytes) {
        return false;
      }
      start = i + 1;
    }
  }
  return true;
}

std::string sanitizeComponent(std::string_view component)
{
  std::string out;
  out.reserve(component.size() + 1);
  for (const char c : component) {
    switch (c) {
    case '\\':
    case '/':
    case ':':
    case '*':
    case '?':
    case '"':
    case '<':
    case '>':
    case '|':
      out.push_back('_');
      break;
    default:
      out.push_back(static_cast<unsigned char>(c) < 0x20 ? '_' : c);
      break;
    }
  }
  while (!out.empty() && (out.back() == '.' || out.back() == ' ')) {
    out.pop_back();
  }
  if (out.empty() || out == "." || out == "..") {
    return "_";
  }
  if (isReservedDeviceName(out)) {
    out.insert(out.begin(), '_');
  }
  return out;
}

std::string sanitizeRelativePath(std::string_view path)
{
  std::string out;
  for (const auto &part : splitPath(path)) {
    if (!out.empty()) {
      out.push_back('/');
    }
    out += sanitizeComponent(part);
  }
  return out;
}

std::string nameKey(std::string_view name)
{
  std::string key(name);
  std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return key;
}

std::string uniqueName(const std::string &name, bool isDirectory, const std::function<bool(const std::string &)> &taken)
{
  if (!taken(name)) {
    return name;
  }

  std::string stem = name;
  std::string extension;
  if (!isDirectory) {
    const auto dot = name.rfind('.');
    if (dot != std::string::npos && dot != 0) {
      stem = name.substr(0, dot);
      extension = name.substr(dot);
    }
  }

  for (uint32_t n = 2; n < 100000; ++n) {
    auto candidate = stem + " (" + std::to_string(n) + ")" + extension;
    if (!taken(candidate)) {
      return candidate;
    }
  }
  // practically unreachable; still never hand back a taken name
  for (uint64_t n = 100000;; ++n) {
    auto candidate = stem + " (" + std::to_string(n) + ")" + extension;
    if (!taken(candidate)) {
      return candidate;
    }
  }
}

bool totalSize(const ItemList &items, uint64_t &total)
{
  total = 0;
  for (const auto &item : items) {
    if (UINT64_MAX - total < item.size) {
      return false;
    }
    total += item.size;
  }
  return true;
}

//
// item lists
//

bool validateItems(const ItemList &items, std::string *error)
{
  if (items.empty()) {
    setError(error, "empty item list");
    return false;
  }
  if (items.size() > kMaxItems) {
    setError(error, "too many items");
    return false;
  }

  std::unordered_set<std::string> directories;
  std::unordered_set<std::string> seen;
  for (size_t i = 0; i < items.size(); ++i) {
    const auto &item = items[i];
    if (!isSafeRelativePath(item.path)) {
      setError(error, "unsafe path in item " + std::to_string(i));
      return false;
    }
    if (item.isDirectory && item.size != 0) {
      setError(error, "folder with a size in item " + std::to_string(i));
      return false;
    }
    if (!seen.insert(item.path).second) {
      setError(error, "duplicate path in item " + std::to_string(i));
      return false;
    }
    const auto parent = parentOf(item.path);
    if (!parent.empty() && !directories.contains(parent)) {
      setError(error, "item " + std::to_string(i) + " listed before its folder");
      return false;
    }
    if (item.isDirectory) {
      directories.insert(item.path);
    }
  }

  uint64_t total = 0;
  if (!totalSize(items, total)) {
    setError(error, "total size overflows");
    return false;
  }
  return true;
}

std::string encodeItems(const ItemList &items)
{
  ByteWriter w;
  w.u32(static_cast<uint32_t>(items.size()));
  for (const auto &item : items) {
    w.u8(item.isDirectory ? 1 : 0);
    w.u64(item.isDirectory ? 0 : item.size);
    w.bytes16(item.path);
  }
  return w.take();
}

bool decodeItems(std::string_view encoded, ItemList &items, std::string *error)
{
  items.clear();
  if (encoded.size() > kMaxItemListBytes) {
    setError(error, "item list too large");
    return false;
  }

  ByteReader r(encoded);
  uint32_t count = 0;
  if (!r.u32(count)) {
    setError(error, "truncated item list");
    return false;
  }
  if (count == 0 || count > kMaxItems) {
    setError(error, "bad item count");
    return false;
  }
  // each item takes at least 11 bytes, so a bogus count can't make us allocate much
  if (count > encoded.size() / 11) {
    setError(error, "truncated item list");
    return false;
  }

  items.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    Item item;
    uint8_t flags = 0;
    if (!r.u8(flags) || !r.u64(item.size) || !r.bytes16(item.path)) {
      items.clear();
      setError(error, "truncated item list");
      return false;
    }
    if ((flags & ~1u) != 0) {
      items.clear();
      setError(error, "unknown item flags");
      return false;
    }
    item.isDirectory = (flags & 1u) != 0;
    items.push_back(std::move(item));
  }
  if (!r.atEnd()) {
    items.clear();
    setError(error, "trailing bytes after item list");
    return false;
  }
  if (!validateItems(items, error)) {
    items.clear();
    return false;
  }
  return true;
}

//
// local transport
//

std::string encodeMessage(const Message &m)
{
  ByteWriter w;
  w.u8(static_cast<uint8_t>(m.type));
  w.u32(m.dragId);
  w.u32(m.index);
  w.u32(m.seq);
  w.u32(m.flags);
  w.u64(m.size);
  w.u8(static_cast<uint8_t>(m.reason));
  w.u16(m.icon.width);
  w.u16(m.icon.height);
  w.bytes32(m.icon.rgba);
  w.bytes32(m.items.empty() ? std::string() : encodeItems(m.items));
  w.bytes32(m.data);
  return w.take();
}

bool decodeMessage(std::string_view encoded, Message &m, std::string *error)
{
  m = Message();
  ByteReader r(encoded);
  uint8_t type = 0;
  uint8_t reason = 0;
  std::string items;
  if (!r.u8(type) || !r.u32(m.dragId) || !r.u32(m.index) || !r.u32(m.seq) || !r.u32(m.flags) || !r.u64(m.size) ||
      !r.u8(reason) || !r.u16(m.icon.width) || !r.u16(m.icon.height) ||
      !r.bytes32(m.icon.rgba, kMaxIconSide * kMaxIconSide * 4) || !r.bytes32(items, kMaxItemListBytes) ||
      !r.bytes32(m.data, kMaxChunkSize) || !r.atEnd()) {
    setError(error, "truncated message");
    return false;
  }
  if (type < static_cast<uint8_t>(Message::Type::Caps) || type > static_cast<uint8_t>(Message::Type::HandOff)) {
    setError(error, "unknown message type");
    return false;
  }
  m.type = static_cast<Message::Type>(type);
  m.reason = cancelReasonFromInt(reason);
  if (!m.icon.isValid()) {
    setError(error, "bad icon");
    return false;
  }
  if (!items.empty() && !decodeItems(items, m.items, error)) {
    return false;
  }
  return true;
}

void unpremultiplyRgba(std::string &rgba)
{
  for (size_t i = 0; i + 3 < rgba.size(); i += 4) {
    const auto a = static_cast<uint8_t>(rgba[i + 3]);
    for (size_t c = 0; c < 3; ++c) {
      if (a == 0) {
        rgba[i + c] = 0;
        continue;
      }
      const unsigned v = (static_cast<unsigned>(static_cast<uint8_t>(rgba[i + c])) * 255u + a / 2) / a;
      rgba[i + c] = static_cast<char>(v > 255 ? 255 : v);
    }
  }
}

} // namespace deskflow::filedrag
