/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ProtocolUtil.h"
#include "base/Log.h"
#include "common/stdvector.h"
#include "deskflow/XDeskflow.h"
#include "deskflow/protocol_types.h"
#include "io/IStream.h"
#include <array>
#include <iterator>

#include <cctype>
#include <cstring>

//
// ProtocolUtil
//

namespace {

void writeInt(uint32_t Value, uint32_t Length, std::vector<uint8_t> &Buffer)
{
  switch (Length) {
  case 1:
    Buffer.push_back(static_cast<uint8_t>(Value & 0xffU));
    break;
  case 4:
    Buffer.push_back(static_cast<uint8_t>((Value >> 24U) & 0xffU));
    Buffer.push_back(static_cast<uint8_t>((Value >> 16U) & 0xffU));
    Buffer.push_back(static_cast<uint8_t>((Value >> 8U) & 0xffU));
    Buffer.push_back(static_cast<uint8_t>(Value & 0xffU));
    break;
  case 2:
    Buffer.push_back(static_cast<uint8_t>((Value >> 8U) & 0xffU));
    Buffer.push_back(static_cast<uint8_t>(Value & 0xffU));
    break;
  default:
    assert(0 && "invalid integer format length");
    return;
  }
}

template <typename T> void writeVectorInt(const std::vector<T> *VectorData, std::vector<uint8_t> &Buffer)
{
  if (VectorData) {
    const std::vector<T> &Vector = *VectorData;
    writeInt((uint32_t)Vector.size(), sizeof(uint32_t), Buffer);
    for (size_t i = 0; i < Vector.size(); ++i) {
      writeInt(Vector[i], sizeof(T), Buffer);
    }
  }
}

void writeString(const std::string *StringData, std::vector<uint8_t> &Buffer)
{
  const uint32_t len = (StringData != NULL) ? (uint32_t)StringData->size() : 0;
  writeInt(len, sizeof(len), Buffer);
  if (len != 0) {
    std::copy(StringData->begin(), StringData->end(), std::back_inserter(Buffer));
  }
}

} // namespace

void ProtocolUtil::writef(deskflow::IStream *stream, const char *fmt, ...)
{
  assert(stream != NULL);
  assert(fmt != NULL);
  LOG((CLOG_DEBUG2 "writef(%s)", fmt));

  va_list args;
  va_start(args, fmt);
  uint32_t size = getLength(fmt, args);
  va_end(args);
  va_start(args, fmt);
  vwritef(stream, fmt, size, args);
  va_end(args);
}

bool ProtocolUtil::readf(deskflow::IStream *stream, const char *fmt, ...)
{
  bool result = false;

  if (stream && fmt) {
    LOG((CLOG_DEBUG2 "readf(%s)", fmt));
    va_list args;
    va_start(args, fmt);
    try {
      vreadf(stream, fmt, args);
      result = true;
    } catch (XIO &) {
      result = false;
    } catch (const std::bad_alloc &) {
      result = false;
    }
    va_end(args);
  }

  return result;
}

void ProtocolUtil::vwritef(deskflow::IStream *stream, const char *fmt, uint32_t size, va_list args)
{
  assert(stream != NULL);
  assert(fmt != NULL);

  // done if nothing to write
  if (size == 0) {
    return;
  }

  // fill buffer
  std::vector<uint8_t> Buffer;
  writef(Buffer, fmt, args);

  try {
    // write buffer
    stream->write(Buffer.data(), size);
    LOG((CLOG_DEBUG2 "wrote %d bytes", size));
  } catch (const XBase &exception) {
    LOG((CLOG_DEBUG2 "exception <%s> during wrote %d bytes into stream", exception.what(), size));
    throw;
  }
}

void ProtocolUtil::vreadf(deskflow::IStream *stream, const char *fmt, va_list args)
{
  assert(stream != NULL);
  assert(fmt != NULL);

  // begin scanning
  while (*fmt) {
    if (*fmt == '%') {
      // format specifier.  determine argument size.
      ++fmt;
      uint32_t len = eatLength(&fmt);
      switch (*fmt) {
      case 'i': {
        void *destination = va_arg(args, void *);
        switch (len) {
        case 1:
          // 1 byte integer
          *static_cast<uint8_t *>(destination) = read1ByteInt(stream);
          break;
        case 2:
          // 2 byte integer
          *static_cast<uint16_t *>(destination) = read2BytesInt(stream);
          break;
        case 4:
          // 4 byte integer
          *static_cast<uint32_t *>(destination) = read4BytesInt(stream);
          break;
        default:
          // the length is wrong
          LOG((CLOG_ERR "read: length to be read is wrong: '%d' should be 1,2, or 4", len));
          assert(false); // assert for debugging
          break;
        }
        break;
      }

      case 'I': {
        void *destination = va_arg(args, void *);
        switch (len) {
        case 1:
          // 1 byte integer
          readVector1ByteInt(stream, *static_cast<std::vector<uint8_t> *>(destination));
          break;
        case 2:
          // 2 byte integer
          readVector2BytesInt(stream, *static_cast<std::vector<uint16_t> *>(destination));
          break;
        case 4:
          // 4 byte integer
          readVector4BytesInt(stream, *static_cast<std::vector<uint32_t> *>(destination));
          break;
        default:
          // the length is wrong
          LOG((CLOG_ERR "read: length to be read is wrong: '%d' should be 1,2, or 4", len));
          assert(false); // assert for debugging
          break;
        }
        break;
      }

      case 's': {
        std::string *destination = va_arg(args, std::string *);

        if (len > PROTOCOL_MAX_STRING_LENGTH) {
          LOG((CLOG_ERR "read: string length exceeds maximum allowed size: %u", len));
          throw XBadClient("Too long message received");
        }

        readBytes(stream, len, destination);
        break;
      }

      case '%':
        assert(len == 0);
        break;

      default:
        assert(0 && "invalid format specifier");
      }

      // next format character
      ++fmt;
    } else {
      // read next character
      char buffer[1];
      read(stream, buffer, 1);

      // verify match
      if (buffer[0] != *fmt) {
        LOG((CLOG_DEBUG2 "readf: format mismatch: %c vs %c", *fmt, buffer[0]));
        throw XIOReadMismatch();
      }

      // next format character
      ++fmt;
    }
  }
}

uint32_t ProtocolUtil::getLength(const char *fmt, va_list args)
{
  uint32_t n = 0;
  while (*fmt) {
    if (*fmt == '%') {
      // format specifier.  determine argument size.
      ++fmt;
      uint32_t len = eatLength(&fmt);
      switch (*fmt) {
      case 'i':
        assert(len == 1 || len == 2 || len == 4);
        (void)va_arg(args, uint32_t);
        break;

      case 'I':
        assert(len == 1 || len == 2 || len == 4);
        switch (len) {
        case 1:
          len = (uint32_t)(va_arg(args, std::vector<uint8_t> *))->size() + 4;
          break;

        case 2:
          len = 2 * (uint32_t)(va_arg(args, std::vector<uint16_t> *))->size() + 4;
          break;

        case 4:
          len = 4 * (uint32_t)(va_arg(args, std::vector<uint32_t> *))->size() + 4;
          break;
        }
        break;

      case 's':
        assert(len == 0);
        len = (uint32_t)(va_arg(args, std::string *))->size() + 4;
        break;

      case 'S':
        assert(len == 0);
        len = va_arg(args, uint32_t) + 4;
        break;

      case '%':
        assert(len == 0);
        len = 1;
        break;

      default:
        assert(0 && "invalid format specifier");
      }

      // accumulate size
      n += len;
      ++fmt;
    } else {
      // regular character
      ++n;
      ++fmt;
    }
  }
  return n;
}

void ProtocolUtil::writef(std::vector<uint8_t> &buffer, const char *fmt, va_list args)
{
  while (*fmt) {
    if (*fmt == '%') {
      // format specifier.  determine argument size.
      ++fmt;
      uint32_t len = eatLength(&fmt);
      switch (*fmt) {
      case 'i': {
        const uint32_t v = va_arg(args, uint32_t);
        writeInt(v, len, buffer);
        break;
      }

      case 'I': {
        switch (len) {
        case 1: {
          // 1 byte integers
          const std::vector<uint8_t> *list = va_arg(args, const std::vector<uint8_t> *);
          writeVectorInt(list, buffer);
          break;
        }

        case 2: {
          // 2 byte integers
          const std::vector<uint16_t> *list = va_arg(args, const std::vector<uint16_t> *);
          writeVectorInt(list, buffer);
          break;
        }

        case 4: {
          // 4 byte integers
          const std::vector<uint32_t> *list = va_arg(args, const std::vector<uint32_t> *);
          writeVectorInt(list, buffer);
          break;
        }

        default:
          assert(0 && "invalid integer vector format length");
          return;
        }
        break;
      }

      case 's': {
        assert(len == 0);
        const std::string *src = va_arg(args, std::string *);
        writeString(src, buffer);
        break;
      }

      case 'S': {
        assert(len == 0);
        const uint32_t len = va_arg(args, uint32_t);
        const uint8_t *src = va_arg(args, uint8_t *);
        writeInt(len, sizeof(len), buffer);
        std::copy(src, src + len, std::back_inserter(buffer));
        break;
      }

      case '%':
        assert(len == 0);
        buffer.push_back('%');
        break;

      default:
        assert(0 && "invalid format specifier");
      }

      // next format character
      ++fmt;
    } else {
      // copy regular character
      buffer.push_back(*fmt++);
    }
  }
}

uint32_t ProtocolUtil::eatLength(const char **pfmt)
{
  const char *fmt = *pfmt;
  uint32_t n = 0;
  for (;;) {
    uint32_t d;
    switch (*fmt) {
    case '0':
      d = 0;
      break;
    case '1':
      d = 1;
      break;
    case '2':
      d = 2;
      break;
    case '3':
      d = 3;
      break;
    case '4':
      d = 4;
      break;
    case '5':
      d = 5;
      break;
    case '6':
      d = 6;
      break;
    case '7':
      d = 7;
      break;
    case '8':
      d = 8;
      break;
    case '9':
      d = 9;
      break;
    default:
      *pfmt = fmt;
      return n;
    }
    n = 10 * n + d;
    ++fmt;
  }
}

void ProtocolUtil::read(deskflow::IStream *stream, void *vbuffer, uint32_t count)
{
  assert(stream != NULL);
  assert(vbuffer != NULL);

  uint8_t *buffer = static_cast<uint8_t *>(vbuffer);
  while (count > 0) {
    // read more
    uint32_t n = stream->read(buffer, count);

    // bail if stream has hungup
    if (n == 0) {
      LOG((CLOG_DEBUG2 "unexpected disconnect in readf(), %d bytes left", count));
      throw XIOEndOfStream();
    }

    // prepare for next read
    buffer += n;
    count -= n;
  }
}

uint8_t ProtocolUtil::read1ByteInt(deskflow::IStream *stream)
{
  const uint32_t BufferSize = 1;
  std::array<uint8_t, 1> buffer = {};
  read(stream, buffer.data(), BufferSize);

  uint8_t Result = buffer[0];
  LOG((CLOG_DEBUG2 "readf: read 1 byte integer: %d (0x%x)", Result, Result));

  return Result;
}

uint16_t ProtocolUtil::read2BytesInt(deskflow::IStream *stream)
{
  const uint32_t BufferSize = 2;
  std::array<uint8_t, BufferSize> buffer = {};
  read(stream, buffer.data(), BufferSize);

  uint16_t Result = static_cast<uint16_t>((static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]));
  LOG((CLOG_DEBUG2 "readf: read 2 byte integer: %d (0x%x)", Result, Result));

  return Result;
}

uint32_t ProtocolUtil::read4BytesInt(deskflow::IStream *stream)
{
  const int BufferSize = 4;
  std::array<uint8_t, BufferSize> buffer = {};
  read(stream, buffer.data(), BufferSize);

  uint32_t Result = (static_cast<uint32_t>(buffer[0]) << 24) | (static_cast<uint32_t>(buffer[1]) << 16) |
                    (static_cast<uint32_t>(buffer[2]) << 8) | (static_cast<uint32_t>(buffer[3]));

  LOG((CLOG_DEBUG2 "readf: read 4 byte integer: %d (0x%x)", Result, Result));

  return Result;
}

void ProtocolUtil::readVector1ByteInt(deskflow::IStream *stream, std::vector<uint8_t> &destination)
{
  uint32_t size = readVectorSize(stream);
  for (uint32_t i = 0; i < size; ++i) {
    destination.push_back(read1ByteInt(stream));
  }
}

void ProtocolUtil::readVector2BytesInt(deskflow::IStream *stream, std::vector<uint16_t> &destination)
{
  uint32_t size = readVectorSize(stream);
  for (uint32_t i = 0; i < size; ++i) {
    destination.push_back(read2BytesInt(stream));
  }
}

void ProtocolUtil::readVector4BytesInt(deskflow::IStream *stream, std::vector<uint32_t> &destination)
{
  uint32_t size = readVectorSize(stream);
  for (uint32_t i = 0; i < size; ++i) {
    destination.push_back(read4BytesInt(stream));
  }
}

uint32_t ProtocolUtil::readVectorSize(deskflow::IStream *stream)
{
  uint32_t size = read4BytesInt(stream);

  if (size > PROTOCOL_MAX_LIST_LENGTH) {
    LOG((CLOG_ERR "readVectorSize: vector length exceeds maximum allowed size: %u", size));
    throw XBadClient("Too long message received");
  }

  return size;
}

void ProtocolUtil::readBytes(deskflow::IStream *stream, uint32_t len, std::string *destination)
{
  // read the string length
  uint8_t buffer[128];

  // when string length is 0, this implies that the size of the string is
  // variable and will be embedded in the stream.
  if (len == 0) {
    len = read4BytesInt(stream);
  }

  // use a fixed size buffer if its big enough
  const bool useFixed = (len <= sizeof(buffer));

  // allocate a buffer to read the data
  uint8_t *sBuffer = buffer;
  if (!useFixed) {
    try {
      sBuffer = new uint8_t[len];
    } catch (std::bad_alloc &exception) {
      // Added try catch due to GHSA-chfm-333q-gfpp
      LOG((CLOG_ERR "bad alloc, unable to allocate memory %d bytes", len));
      LOG((CLOG_DEBUG "bad_alloc detected: is there enough memory?"));
      throw exception;
    }
  }

  // read the data
  try {
    read(stream, sBuffer, len);
  } catch (...) {
    if (!useFixed) {
      delete[] sBuffer;
    }
    throw;
  }

  LOG((CLOG_DEBUG2 "readf: read %d byte string", len));

  // save the data

  if (destination) {
    destination->assign((const char *)sBuffer, len);
  }

  // release the buffer
  if (!useFixed) {
    delete[] sBuffer;
  }
}

//
// XIOReadMismatch
//

std::string XIOReadMismatch::getWhat() const throw()
{
  return format("XIOReadMismatch", "ProtocolUtil::readf() mismatch");
}
