/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Unicode.h"

//
// local utility functions
//
inline static uint16_t decode16(const uint8_t *n, bool byteSwapped)
{
  union x16
  {
    uint8_t n8[2];
    uint16_t n16;
  };
  x16 c;

  if (byteSwapped) {
    c.n8[0] = n[1];
    c.n8[1] = n[0];
  } else {
    c.n8[0] = n[0];
    c.n8[1] = n[1];
  }
  return c.n16;
}

inline static uint32_t decode32(const uint8_t *n, bool byteSwapped)
{
  union x32
  {
    uint8_t n8[4];
    uint32_t n32;
  };
  x32 c;
  if (byteSwapped) {
    c.n8[0] = n[3];
    c.n8[1] = n[2];
    c.n8[2] = n[1];
    c.n8[3] = n[0];
  } else {
    c.n8[0] = n[0];
    c.n8[1] = n[1];
    c.n8[2] = n[2];
    c.n8[3] = n[3];
  }
  return c.n32;
}

inline static void resetError(bool *errors)
{
  if (errors != nullptr) {
    *errors = false;
  }
}

inline static void setError(bool *errors)
{
  if (errors != nullptr) {
    *errors = true;
  }
}

//
// Unicode
//

uint32_t Unicode::s_invalid = 0x0000ffff;
uint32_t Unicode::s_replacement = 0x0000fffd;

bool Unicode::isUTF8(const std::string &src)
{
  // convert and test each character
  const auto *data = reinterpret_cast<const uint8_t *>(src.c_str());
  for (auto n = (uint32_t)src.size(); n > 0;) {
    if (fromUTF8(data, n) == s_invalid) {
      return false;
    }
  }
  return true;
}

std::string Unicode::UTF8ToUCS2(const std::string &src, bool *errors)
{
  // default to success
  resetError(errors);

  // get size of input string and reserve some space in output
  auto n = (uint32_t)src.size();
  std::string dst;
  dst.reserve(2 * n);

  // convert each character
  const auto *data = reinterpret_cast<const uint8_t *>(src.c_str());
  while (n > 0) {
    uint32_t c = fromUTF8(data, n);
    if (c == s_invalid) {
      c = s_replacement;
    } else if (c >= 0x00010000) {
      setError(errors);
      c = s_replacement;
    }
    auto ucs2 = static_cast<uint16_t>(c);
    dst.append(reinterpret_cast<const char *>(&ucs2), 2);
  }

  return dst;
}

std::string Unicode::UTF8ToUCS4(const std::string &src, bool *errors)
{
  // default to success
  resetError(errors);

  // get size of input string and reserve some space in output
  auto n = (uint32_t)src.size();
  std::string dst;
  dst.reserve(4 * n);

  // convert each character
  const auto *data = reinterpret_cast<const uint8_t *>(src.c_str());
  while (n > 0) {
    uint32_t c = fromUTF8(data, n);
    if (c == s_invalid) {
      c = s_replacement;
    }
    dst.append(reinterpret_cast<const char *>(&c), 4);
  }

  return dst;
}

std::string Unicode::UTF8ToUTF16(const std::string &src, bool *errors)
{
  // default to success
  resetError(errors);

  // get size of input string and reserve some space in output
  auto n = (uint32_t)src.size();
  std::string dst;
  dst.reserve(2 * n);

  // convert each character
  const auto *data = reinterpret_cast<const uint8_t *>(src.c_str());
  while (n > 0) {
    uint32_t c = fromUTF8(data, n);
    if (c == s_invalid) {
      c = s_replacement;
    } else if (c >= 0x00110000) {
      setError(errors);
      c = s_replacement;
    }
    if (c < 0x00010000) {
      auto ucs2 = static_cast<uint16_t>(c);
      dst.append(reinterpret_cast<const char *>(&ucs2), 2);
    } else {
      c -= 0x00010000;
      auto utf16h = static_cast<uint16_t>((c >> 10) + 0xd800);
      auto utf16l = static_cast<uint16_t>((c & 0x03ff) + 0xdc00);
      dst.append(reinterpret_cast<const char *>(&utf16h), 2);
      dst.append(reinterpret_cast<const char *>(&utf16l), 2);
    }
  }

  return dst;
}

std::string Unicode::UTF8ToUTF32(const std::string &src, bool *errors)
{
  // default to success
  resetError(errors);

  // get size of input string and reserve some space in output
  auto n = (uint32_t)src.size();
  std::string dst;
  dst.reserve(4 * n);

  // convert each character
  const auto *data = reinterpret_cast<const uint8_t *>(src.c_str());
  while (n > 0) {
    uint32_t c = fromUTF8(data, n);
    if (c == s_invalid) {
      c = s_replacement;
    } else if (c >= 0x00110000) {
      setError(errors);
      c = s_replacement;
    }
    dst.append(reinterpret_cast<const char *>(&c), 4);
  }

  return dst;
}

std::string Unicode::UCS2ToUTF8(const std::string_view &src, bool *errors)
{
  // default to success
  resetError(errors);

  // convert
  uint32_t n = (uint32_t)src.size() >> 1;
  return doUCS2ToUTF8(reinterpret_cast<const uint8_t *>(src.data()), n, errors);
}

std::string Unicode::UCS4ToUTF8(const std::string_view &src, bool *errors)
{
  // default to success
  resetError(errors);

  // convert
  uint32_t n = (uint32_t)src.size() >> 2;
  return doUCS4ToUTF8(reinterpret_cast<const uint8_t *>(src.data()), n, errors);
}

std::string Unicode::UTF16ToUTF8(const std::string_view &src, bool *errors)
{
  // default to success
  resetError(errors);

  // convert
  uint32_t n = (uint32_t)src.size() >> 1;
  return doUTF16ToUTF8(reinterpret_cast<const uint8_t *>(src.data()), n, errors);
}

std::string Unicode::UTF32ToUTF8(const std::string_view &src, bool *errors)
{
  // default to success
  resetError(errors);

  // convert
  uint32_t n = (uint32_t)src.size() >> 2;
  return doUTF32ToUTF8(reinterpret_cast<const uint8_t *>(src.data()), n, errors);
}

std::string Unicode::doUCS2ToUTF8(const uint8_t *data, uint32_t n, bool *errors)
{
  // make some space
  std::string dst;
  dst.reserve(n);

  // check if first character is 0xfffe or 0xfeff
  bool byteSwapped = false;
  if (n >= 1) {
    switch (decode16(data, false)) {
    case 0x0000feff:
      data += 2;
      --n;
      break;

    case 0x0000fffe:
      byteSwapped = true;
      data += 2;
      --n;
      break;

    default:
      break;
    }
  }

  // convert each character
  for (; n > 0; --n) {
    uint32_t c = decode16(data, byteSwapped);
    toUTF8(dst, c, errors);
    data += 2;
  }

  return dst;
}

std::string Unicode::doUCS4ToUTF8(const uint8_t *data, uint32_t n, bool *errors)
{
  // make some space
  std::string dst;
  dst.reserve(n);

  // check if first character is 0xfffe or 0xfeff
  bool byteSwapped = false;
  if (n >= 1) {
    switch (decode32(data, false)) {
    case 0x0000feff:
      data += 4;
      --n;
      break;

    case 0x0000fffe:
      byteSwapped = true;
      data += 4;
      --n;
      break;

    default:
      break;
    }
  }

  // convert each character
  for (; n > 0; --n) {
    auto c = decode32(data, byteSwapped);
    toUTF8(dst, c, errors);
    data += 4;
  }

  return dst;
}

std::string Unicode::doUTF16ToUTF8(const uint8_t *data, uint32_t n, bool *errors)
{
  // make some space
  std::string dst;
  dst.reserve(n);

  // check if first character is 0xfffe or 0xfeff
  bool byteSwapped = false;
  if (n >= 1) {
    switch (decode16(data, false)) {
    case 0x0000feff:
      data += 2;
      --n;
      break;

    case 0x0000fffe:
      byteSwapped = true;
      data += 2;
      --n;
      break;

    default:
      break;
    }
  }

  // convert each character
  while (n > 0) {
    if (uint32_t c = decode16(data, byteSwapped); c < 0x0000d800 || c > 0x0000dfff) {
      toUTF8(dst, c, errors);
    } else if (n == 1) {
      // error -- missing second word
      setError(errors);
      toUTF8(dst, s_replacement, nullptr);
    } else if (c >= 0x0000d800 && c <= 0x0000dbff) {
      data += 2;
      --n;
      if (uint32_t c2 = decode16(data, byteSwapped); c2 < 0x0000dc00 || c2 > 0x0000dfff) {
        // error -- [d800,dbff] not followed by [dc00,dfff]
        setError(errors);
        toUTF8(dst, s_replacement, nullptr);
      } else {
        c = (((c - 0x0000d800) << 10) | (c2 - 0x0000dc00)) + 0x00010000;
        toUTF8(dst, c, errors);
      }
    } else {
      // error -- [dc00,dfff] without leading [d800,dbff]
      setError(errors);
      toUTF8(dst, s_replacement, nullptr);
    }
    data += 2;
    --n;
  }

  return dst;
}

std::string Unicode::doUTF32ToUTF8(const uint8_t *data, uint32_t n, bool *errors)
{
  // make some space
  std::string dst;
  dst.reserve(n);

  // check if first character is 0xfffe or 0xfeff
  bool byteSwapped = false;
  if (n >= 1) {
    switch (decode32(data, false)) {
    case 0x0000feff:
      data += 4;
      --n;
      break;

    case 0x0000fffe:
      byteSwapped = true;
      data += 4;
      --n;
      break;

    default:
      break;
    }
  }

  // convert each character
  for (; n > 0; --n) {
    auto c = decode32(data, byteSwapped);
    if (c >= 0x00110000) {
      setError(errors);
      c = s_replacement;
    }
    toUTF8(dst, c, errors);
    data += 4;
  }

  return dst;
}

uint32_t Unicode::fromUTF8(const uint8_t *&data, uint32_t &n)
{
  assert(data != nullptr);
  assert(n != 0);

  // compute character encoding length, checking for overlong
  // sequences (i.e. characters that don't use the shortest
  // possible encoding).
  uint32_t size;
  if (data[0] < 0x80) {
    // 0xxxxxxx
    size = 1;
  } else if (data[0] < 0xc0) {
    // 10xxxxxx -- in the middle of a multibyte character.  counts
    // as one invalid character.
    --n;
    ++data;
    return s_invalid;
  } else if (data[0] < 0xe0) {
    // 110xxxxx
    size = 2;
  } else if (data[0] < 0xf0) {
    // 1110xxxx
    size = 3;
  } else if (data[0] < 0xf8) {
    // 11110xxx
    size = 4;
  } else if (data[0] < 0xfc) {
    // 111110xx
    size = 5;
  } else if (data[0] < 0xfe) {
    // 1111110x
    size = 6;
  } else {
    // invalid sequence.  dunno how many bytes to skip so skip one.
    --n;
    ++data;
    return s_invalid;
  }

  // make sure we have enough data
  if (size > n) {
    data += n;
    n = 0;
    return s_invalid;
  }

  // extract character
  uint32_t c;
  switch (size) {
  case 1:
    c = static_cast<uint32_t>(data[0]);
    break;

  case 2:
    c = ((static_cast<uint32_t>(data[0]) & 0x1f) << 6) | (static_cast<uint32_t>(data[1]) & 0x3f);
    break;

  case 3:
    c = ((static_cast<uint32_t>(data[0]) & 0x0f) << 12) | ((static_cast<uint32_t>(data[1]) & 0x3f) << 6) |
        (static_cast<uint32_t>(data[2]) & 0x3f);
    break;

  case 4:
    c = ((static_cast<uint32_t>(data[0]) & 0x07) << 18) | ((static_cast<uint32_t>(data[1]) & 0x3f) << 12) |
        ((static_cast<uint32_t>(data[2]) & 0x3f) << 6) | (static_cast<uint32_t>(data[3]) & 0x3f);
    break;

  case 5:
    c = ((static_cast<uint32_t>(data[0]) & 0x03) << 24) | ((static_cast<uint32_t>(data[1]) & 0x3f) << 18) |
        ((static_cast<uint32_t>(data[2]) & 0x3f) << 12) | ((static_cast<uint32_t>(data[3]) & 0x3f) << 6) |
        (static_cast<uint32_t>(data[4]) & 0x3f);
    break;

  case 6:
    c = ((static_cast<uint32_t>(data[0]) & 0x01) << 30) | ((static_cast<uint32_t>(data[1]) & 0x3f) << 24) |
        ((static_cast<uint32_t>(data[2]) & 0x3f) << 18) | ((static_cast<uint32_t>(data[3]) & 0x3f) << 12) |
        ((static_cast<uint32_t>(data[4]) & 0x3f) << 6) | (static_cast<uint32_t>(data[5]) & 0x3f);
    break;

  default:
    assert(0 && "invalid size");
    return s_invalid;
  }

  // check that all bytes after the first have the pattern 10xxxxxx.
  // truncated sequences are treated as a single malformed character.
  bool truncated = false;
  switch (size) {
  case 6:
    if ((data[5] & 0xc0) != 0x80) {
      truncated = true;
      size = 5;
    }
    // fall through

  case 5:
    if ((data[4] & 0xc0) != 0x80) {
      truncated = true;
      size = 4;
    }
    // fall through

  case 4:
    if ((data[3] & 0xc0) != 0x80) {
      truncated = true;
      size = 3;
    }
    // fall through

  case 3:
    if ((data[2] & 0xc0) != 0x80) {
      truncated = true;
      size = 2;
    }
    // fall through

  case 2:
    if ((data[1] & 0xc0) != 0x80) {
      truncated = true;
      size = 1;
    }

  default:
    break;
  }

  // update parameters
  data += size;
  n -= size;

  // invalid if sequence was truncated
  if (truncated) {
    return s_invalid;
  }

  // check for characters that didn't use the smallest possible encoding
  if (static uint32_t s_minChar[] = {0, 0x00000000, 0x00000080, 0x00000800, 0x00010000, 0x00200000, 0x04000000};
      c < s_minChar[size]) {
    return s_invalid;
  }

  // check for characters not in ISO-10646
  if (c >= 0x0000d800 && c <= 0x0000dfff) {
    return s_invalid;
  }
  if (c >= 0x0000fffe && c <= 0x0000ffff) {
    return s_invalid;
  }

  return c;
}

void Unicode::toUTF8(std::string &dst, uint32_t c, bool *errors)
{
  uint8_t data[6];

  // handle characters outside the valid range
  if ((c >= 0x0000d800 && c <= 0x0000dfff) || c >= 0x80000000) {
    setError(errors);
    c = s_replacement;
  }

  // convert to UTF-8
  if (c < 0x00000080) {
    data[0] = static_cast<uint8_t>(c);
    dst.append(reinterpret_cast<char *>(data), 1);
  } else if (c < 0x00000800) {
    data[0] = static_cast<uint8_t>(((c >> 6) & 0x0000001f) + 0xc0);
    data[1] = static_cast<uint8_t>((c & 0x0000003f) + 0x80);
    dst.append(reinterpret_cast<char *>(data), 2);
  } else if (c < 0x00010000) {
    data[0] = static_cast<uint8_t>(((c >> 12) & 0x0000000f) + 0xe0);
    data[1] = static_cast<uint8_t>(((c >> 6) & 0x0000003f) + 0x80);
    data[2] = static_cast<uint8_t>((c & 0x0000003f) + 0x80);
    dst.append(reinterpret_cast<char *>(data), 3);
  } else if (c < 0x00200000) {
    data[0] = static_cast<uint8_t>(((c >> 18) & 0x00000007) + 0xf0);
    data[1] = static_cast<uint8_t>(((c >> 12) & 0x0000003f) + 0x80);
    data[2] = static_cast<uint8_t>(((c >> 6) & 0x0000003f) + 0x80);
    data[3] = static_cast<uint8_t>((c & 0x0000003f) + 0x80);
    dst.append(reinterpret_cast<char *>(data), 4);
  } else if (c < 0x04000000) {
    data[0] = static_cast<uint8_t>(((c >> 24) & 0x00000003) + 0xf8);
    data[1] = static_cast<uint8_t>(((c >> 18) & 0x0000003f) + 0x80);
    data[2] = static_cast<uint8_t>(((c >> 12) & 0x0000003f) + 0x80);
    data[3] = static_cast<uint8_t>(((c >> 6) & 0x0000003f) + 0x80);
    data[4] = static_cast<uint8_t>((c & 0x0000003f) + 0x80);
    dst.append(reinterpret_cast<char *>(data), 5);
  } else if (c < 0x80000000) {
    data[0] = static_cast<uint8_t>(((c >> 30) & 0x00000001) + 0xfc);
    data[1] = static_cast<uint8_t>(((c >> 24) & 0x0000003f) + 0x80);
    data[2] = static_cast<uint8_t>(((c >> 18) & 0x0000003f) + 0x80);
    data[3] = static_cast<uint8_t>(((c >> 12) & 0x0000003f) + 0x80);
    data[4] = static_cast<uint8_t>(((c >> 6) & 0x0000003f) + 0x80);
    data[5] = static_cast<uint8_t>((c & 0x0000003f) + 0x80);
    dst.append(reinterpret_cast<char *>(data), 6);
  } else {
    assert(0 && "character out of range");
  }
}
