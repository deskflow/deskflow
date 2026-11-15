/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/FileDragProtocol.h"

#include "deskflow/FileDragCodec.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"

#include <array>
#include <cstring>

namespace deskflow::filedrag {

namespace {

struct CodeEntry
{
  const char *const *format;
  Message::Type type;
};

const std::array<CodeEntry, 13> &codes()
{
  static const std::array<CodeEntry, 13> table = {{
      {&kMsgDFileDragCaps, Message::Type::Caps},
      {&kMsgDFileDragIcon, Message::Type::Icon},
      {&kMsgDFileDragBegin, Message::Type::Begin},
      {&kMsgDFileDragStart, Message::Type::FileStart},
      {&kMsgDFileDragChunk, Message::Type::Chunk},
      {&kMsgDFileDragEnd, Message::Type::FileEnd},
      {&kMsgDFileDragAck, Message::Type::Ack},
      {&kMsgDFileDragCancel, Message::Type::Cancel},
      {&kMsgDFileDragDropped, Message::Type::Dropped},
      {&kMsgDFileDragFinished, Message::Type::Finished},
      {&kMsgDFileDragWatch, Message::Type::Watch},
      {&kMsgDFileDragOffer, Message::Type::Offer},
      {&kMsgDFileDragHandOff, Message::Type::HandOff},
  }};
  return table;
}

bool fail(std::string *error, const char *message)
{
  if (error != nullptr) {
    *error = message;
  }
  return false;
}

} // namespace

bool isFileDragCode(const uint8_t *code)
{
  for (const auto &entry : codes()) {
    if (memcmp(code, *entry.format, 4) == 0) {
      return true;
    }
  }
  return false;
}

void writeMessage(deskflow::IStream *stream, const Message &m)
{
  switch (m.type) {
    using enum Message::Type;
  case Caps:
    ProtocolUtil::writef(stream, kMsgDFileDragCaps, m.flags);
    break;
  case Icon: {
    ProtocolUtil::writef(stream, kMsgDFileDragIcon, m.dragId, m.icon.width, m.icon.height, &m.icon.rgba);
    break;
  }
  case Begin: {
    const auto items = encodeItems(m.items);
    ProtocolUtil::writef(stream, kMsgDFileDragBegin, m.dragId, &items);
    break;
  }
  case FileStart: {
    const auto hi = static_cast<uint32_t>(m.size >> 32);
    const auto lo = static_cast<uint32_t>(m.size & 0xffffffffu);
    ProtocolUtil::writef(stream, kMsgDFileDragStart, m.dragId, m.index, hi, lo);
    break;
  }
  case Chunk:
    ProtocolUtil::writef(stream, kMsgDFileDragChunk, m.dragId, m.index, m.seq, &m.data);
    break;
  case FileEnd:
    ProtocolUtil::writef(stream, kMsgDFileDragEnd, m.dragId, m.index);
    break;
  case Ack:
    ProtocolUtil::writef(stream, kMsgDFileDragAck, m.dragId, m.seq);
    break;
  case Cancel:
    ProtocolUtil::writef(stream, kMsgDFileDragCancel, m.dragId, static_cast<uint32_t>(m.reason));
    break;
  case Dropped:
    ProtocolUtil::writef(stream, kMsgDFileDragDropped, m.dragId);
    break;
  case Finished:
    ProtocolUtil::writef(stream, kMsgDFileDragFinished, m.dragId);
    break;
  case Watch:
    ProtocolUtil::writef(stream, kMsgDFileDragWatch, m.flags);
    break;
  case Offer:
    ProtocolUtil::writef(stream, kMsgDFileDragOffer, m.flags != 0 ? 1u : 0u);
    break;
  case HandOff:
    ProtocolUtil::writef(stream, kMsgDFileDragHandOff, m.dragId);
    break;
  }
}

bool readMessage(deskflow::IStream *stream, const uint8_t *code, Message &m, std::string *error)
{
  m = Message();
  const CodeEntry *entry = nullptr;
  for (const auto &candidate : codes()) {
    if (memcmp(code, *candidate.format, 4) == 0) {
      entry = &candidate;
      break;
    }
  }
  if (entry == nullptr) {
    return fail(error, "not a file drag message");
  }
  m.type = entry->type;
  const char *args = *entry->format + 4;

  switch (m.type) {
    using enum Message::Type;
  case Caps:
    if (!ProtocolUtil::readf(stream, args, &m.flags)) {
      return fail(error, "truncated caps");
    }
    return true;

  case Icon: {
    uint32_t width = 0;
    uint32_t height = 0;
    if (!ProtocolUtil::readf(stream, args, &m.dragId, &width, &height, &m.icon.rgba)) {
      return fail(error, "truncated icon");
    }
    m.icon.width = static_cast<uint16_t>(width);
    m.icon.height = static_cast<uint16_t>(height);
    if (!m.icon.isValid()) {
      return fail(error, "bad icon size");
    }
    return true;
  }

  case Begin: {
    std::string items;
    if (!ProtocolUtil::readf(stream, args, &m.dragId, &items)) {
      return fail(error, "truncated begin");
    }
    return decodeItems(items, m.items, error);
  }

  case FileStart: {
    uint32_t hi = 0;
    uint32_t lo = 0;
    if (!ProtocolUtil::readf(stream, args, &m.dragId, &m.index, &hi, &lo)) {
      return fail(error, "truncated file start");
    }
    m.size = (static_cast<uint64_t>(hi) << 32) | lo;
    return true;
  }

  case Chunk:
    if (!ProtocolUtil::readf(stream, args, &m.dragId, &m.index, &m.seq, &m.data)) {
      return fail(error, "truncated chunk");
    }
    if (m.data.size() > kMaxChunkSize) {
      return fail(error, "chunk too large");
    }
    return true;

  case FileEnd:
    if (!ProtocolUtil::readf(stream, args, &m.dragId, &m.index)) {
      return fail(error, "truncated file end");
    }
    return true;

  case Ack:
    if (!ProtocolUtil::readf(stream, args, &m.dragId, &m.seq)) {
      return fail(error, "truncated ack");
    }
    return true;

  case Cancel: {
    uint32_t reason = 0;
    if (!ProtocolUtil::readf(stream, args, &m.dragId, &reason)) {
      return fail(error, "truncated cancel");
    }
    m.reason = cancelReasonFromInt(reason);
    return true;
  }

  case Dropped:
  case Finished:
  case HandOff:
    if (!ProtocolUtil::readf(stream, args, &m.dragId)) {
      return fail(error, "truncated message");
    }
    return true;

  case Watch:
    if (!ProtocolUtil::readf(stream, args, &m.flags)) {
      return fail(error, "truncated watch");
    }
    if ((m.flags & ~0x1eu) != 0) {
      return fail(error, "bad watch sides");
    }
    return true;

  case Offer: {
    uint8_t ready = 0;
    if (!ProtocolUtil::readf(stream, args, &ready)) {
      return fail(error, "truncated offer");
    }
    if (ready > 1) {
      return fail(error, "bad offer");
    }
    m.flags = ready;
    return true;
  }
  }
  return fail(error, "unknown file drag message");
}

} // namespace deskflow::filedrag
