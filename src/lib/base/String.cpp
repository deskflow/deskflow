/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/String.h"

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <vector>

namespace deskflow::string {

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
          if (static_cast<size_t>(i) > maxIndex) {
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
  for (size_t i = 0; i < maxIndex; ++i) {
    const char *arg = va_arg(args, const char *);
    size_t len = strnlen(arg, SIZE_MAX);
    value.push_back(arg);
    length.push_back(len);
  }

  // compute final length
  size_t resultLength = fmtLength;
  const auto n = static_cast<int>(pos.size());
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
  auto len = static_cast<int>(std::size(tmp));

  std::string result;
  while (buffer != nullptr) {
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
      buffer = nullptr;
    }
  }

  return result;
}

size_t stringToSizeType(const std::string &string)
{
  std::istringstream iss(string);
  size_t value;
  iss >> value;
  return value;
}

//
// CaselessCmp
//

bool CaselessCmp::operator()(const std::string &a, const std::string &b) const
{
  return less(a, b);
}

bool CaselessCmp::less(const std::string_view &a, const std::string_view &b)
{
  return std::ranges::lexicographical_compare(a, b, &deskflow::string::CaselessCmp::cmpLess);
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

} // namespace deskflow::string
