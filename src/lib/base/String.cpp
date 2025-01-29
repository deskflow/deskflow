/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/String.h"
#include "common/stdvector.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdio.h>
#include <string>

namespace deskflow {
namespace string {

std::string format(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  std::string result = vformat(fmt, args);
  va_end(args);
  return result;
}

std::string vformat(const char *fmt, va_list args)
{
  // find highest indexed substitution and the locations of substitutions
  std::vector<size_t> pos;
  std::vector<size_t> width;
  std::vector<size_t> index;
  size_t maxIndex = 0;
  size_t fmtLength = 0; // To store the length of fmt
  const char *scan = fmt;
  while (*scan) {
    if (*scan == '%') {
      ++scan;
      if (*scan == '\0') {
        break;
      } else if (*scan == '%') {
        // literal
        index.push_back(0);
        pos.push_back(static_cast<size_t>((scan - 1) - fmt));
        width.push_back(2);
      } else if (*scan == '{') {
        // get argument index
        char *end;
        errno = 0;
        long i = strtol(scan + 1, &end, 10);
        if (errno || (i < 0) || (*end != '}')) {
          // invalid index -- ignore
          scan = end - 1; // BUG if there are digits?
        } else {
          index.push_back(i);
          pos.push_back(static_cast<size_t>((scan - 1) - fmt));
          width.push_back(static_cast<size_t>((end - scan) + 2));
          if (i > maxIndex) {
            maxIndex = i;
          }
          scan = end;
        }
      } else {
        // improper escape -- ignore
      }
    }
    ++scan;
    ++fmtLength; // Increment fmtLength for each character processed
  }

  // get args
  std::vector<const char *> value;
  std::vector<size_t> length;
  value.push_back("%");
  length.push_back(1);
  for (int i = 0; i < maxIndex; ++i) {
    const char *arg = va_arg(args, const char *);
    size_t len = strnlen(arg, SIZE_MAX);
    value.push_back(arg);
    length.push_back(len);
  }

  // compute final length
  size_t resultLength = fmtLength;
  const int n = static_cast<int>(pos.size());
  for (int i = 0; i < n; ++i) {
    resultLength -= width[i];
    resultLength += length[index[i]];
  }

  // substitute
  std::string result;
  result.reserve(resultLength);
  size_t src = 0;
  for (int i = 0; i < n; ++i) {
    result.append(fmt + src, pos[i] - src);
    result.append(value[index[i]]);
    src = pos[i] + width[i];
  }
  result.append(fmt + src);

  return result;
}

std::string sprintf(const char *fmt, ...)
{
  char tmp[1024];
  char *buffer = tmp;
  int len = (int)(sizeof(tmp) / sizeof(tmp[0]));
  std::string result;
  while (buffer != NULL) {
    // try printing into the buffer
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buffer, len, fmt, args);
    va_end(args);

    // if the buffer wasn't big enough then make it bigger and try again
    if (n < 0 || n > len) {
      if (buffer != tmp) {
        delete[] buffer;
      }
      len *= 2;
      buffer = new char[len];
    }

    // if it was big enough then save the string and don't try again
    else {
      result = buffer;
      if (buffer != tmp) {
        delete[] buffer;
      }
      buffer = NULL;
    }
  }

  return result;
}

void findReplaceAll(std::string &subject, const std::string &find, const std::string &replace)
{
  size_t pos = 0;
  while ((pos = subject.find(find, pos)) != std::string::npos) {
    subject.replace(pos, find.length(), replace);
    pos += replace.length();
  }
}

std::string removeFileExt(std::string filename)
{
  size_t dot = filename.find_last_of('.');

  if (dot == std::string::npos) {
    return filename;
  }

  return filename.substr(0, dot);
}

std::string toHex(const std::string &subject, int width, const char fill)
{
  std::stringstream ss;
  ss << std::hex;
  for (unsigned int i = 0; i < subject.length(); i++) {
    ss << std::setw(width) << std::setfill(fill) << (int)(unsigned char)subject[i];
  }

  return ss.str();
}

std::string toHex(const std::vector<uint8_t> &input, int width, const char fill)
{
  std::stringstream ss;
  ss << std::hex;
  for (unsigned int i = 0; i < input.size(); i++) {
    ss << std::setw(width) << std::setfill(fill) << static_cast<int>(input[i]);
  }

  return ss.str();
}

// clang-format off
int fromHexChar(char c)
{
  switch (c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a':
    case 'A': return 10;
    case 'b':
    case 'B': return 11;
    case 'c':
    case 'C': return 12;
    case 'd':
    case 'D': return 13;
    case 'e':
    case 'E': return 14;
    case 'f':
    case 'F': return 15;
    default:  return -1;
  }
  return -1;
}
// clang-format on

std::vector<uint8_t> fromHex(const std::string &hexString)
{
  std::vector<uint8_t> result;
  result.reserve(hexString.size() / 2);

  size_t i = 0;
  while (i < hexString.size()) {
    if (hexString[i] == ':') {
      i++;
      continue;
    }

    if (i + 2 > hexString.size()) {
      return {}; // uneven character count follows, it's unclear how to interpret it
    }

    auto high = fromHexChar(hexString[i]);
    auto low = fromHexChar(hexString[i + 1]);
    if (high < 0 || low < 0) {
      return {};
    }
    result.push_back(high * 16 + low);
    i += 2;
  }
  return result;
}

void uppercase(std::string &subject)
{
  std::transform(subject.begin(), subject.end(), subject.begin(), ::toupper);
}

void removeChar(std::string &subject, const char c)
{
  subject.erase(std::remove(subject.begin(), subject.end(), c), subject.end());
}

std::string sizeTypeToString(size_t n)
{
  std::stringstream ss;
  ss << n;
  return ss.str();
}

size_t stringToSizeType(std::string string)
{
  std::istringstream iss(string);
  size_t value;
  iss >> value;
  return value;
}

std::vector<std::string> splitString(std::string string, const char c)
{
  std::vector<std::string> results;

  size_t head = 0;
  size_t separator = string.find(c);
  while (separator != std::string::npos) {
    if (head != separator) {
      results.push_back(string.substr(head, separator - head));
    }
    head = separator + 1;
    separator = string.find(c, head);
  }

  if (head < string.size()) {
    results.push_back(string.substr(head, string.size() - head));
  }

  return results;
}

//
// CaselessCmp
//

bool CaselessCmp::operator()(const std::string &a, const std::string &b) const
{
  return less(a, b);
}

bool CaselessCmp::less(const std::string &a, const std::string &b)
{
  return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), &deskflow::string::CaselessCmp::cmpLess);
}

bool CaselessCmp::equal(const std::string &a, const std::string &b)
{
  return !(less(a, b) || less(b, a));
}

bool CaselessCmp::cmpLess(const std::string::value_type &a, const std::string::value_type &b)
{
  // should use std::tolower but not in all versions of libstdc++ have it
  return tolower(a) < tolower(b);
}

bool CaselessCmp::cmpEqual(const std::string::value_type &a, const std::string::value_type &b)
{
  // should use std::tolower but not in all versions of libstdc++ have it
  return tolower(a) == tolower(b);
}

} // namespace string
} // namespace deskflow
