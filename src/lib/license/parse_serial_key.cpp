/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "parse_serial_key.h"

#include "SerialKey.h"
#include "SerialKeyType.h"

#include <charconv>
#include <string>
#include <vector>

using Parts = std::vector<std::string>;

namespace synergy::license {

std::string decode(const std::string &hexString);
Parts splitToParts(const std::string &plainText);
SerialKey parseV1(const std::string &hexString, const Parts &parts);
SerialKey parseV2(const std::string &hexString, const Parts &parts);
long long parseDate(const std::string_view &s);

SerialKey parseSerialKey(const std::string &hexString) {
  auto plainText = decode(hexString);
  const auto parts = splitToParts(plainText);

  if ((parts.size() == 8) && (parts.at(0).find("v1") != std::string::npos)) {
    return parseV1(hexString, parts);
  } else if (
      (parts.size() == 9) && (parts.at(0).find("v2") != std::string::npos)) {
    return parseV2(hexString, parts);
  } else {
    throw InvalidSerialKeyVersion();
  }
}

std::string decode(const std::string &hexString) {
  static const char *const lut = "0123456789ABCDEF";
  std::string plainText;
  size_t len = hexString.length();
  if (len & 1) {
    return plainText;
  }

  plainText.reserve(len / 2);
  for (size_t i = 0; i < len; i += 2) {

    char a = hexString[i];
    char b = hexString[i + 1];

    const auto p = std::lower_bound(lut, lut + 16, a);
    const auto q = std::lower_bound(lut, lut + 16, b);

    if (*q != b || *p != a) {
      return plainText;
    }

    plainText.push_back(static_cast<char>(((p - lut) << 4) | (q - lut)));
  }

  return plainText;
}

SerialKey parseV1(const std::string &hexString, const Parts &parts) {
  // e.g.: {v1;basic;name;1;email;company name;1398297600;1398384000}
  SerialKey serialKey;
  serialKey.hexString = hexString;
  serialKey.product = Product(parts.at(1));
  serialKey.warnTime = parseDate(parts.at(6));
  serialKey.expireTime = parseDate(parts.at(7));
  serialKey.isValid = true;
  return serialKey;
}

SerialKey parseV2(const std::string &hexString, const Parts &parts) {
  // e.g.: {v2;trial;basic;name;1;email;company name;1398297600;1398384000}
  SerialKey serialKey;
  serialKey.hexString = hexString;
  serialKey.type = SerialKeyType(parts.at(1));
  serialKey.product = Product(parts.at(2));
  serialKey.warnTime = parseDate(parts.at(7));
  serialKey.expireTime = parseDate(parts.at(8));
  serialKey.isValid = true;
  return serialKey;
}

Parts splitToParts(const std::string &plainText) {
  // tokenize serialised subscription.
  Parts parts;

  if (!plainText.empty()) {
    std::string parityStart = plainText.substr(0, 1);
    std::string parityEnd = plainText.substr(plainText.length() - 1, 1);

    // check for parity chars { and }, record parity result, then remove them.
    if (parityStart == "{" && parityEnd == "}") {
      const auto serialData = plainText.substr(1, plainText.length() - 2);

      std::string::size_type pos = 0;
      bool look = true;
      while (look) {
        std::string::size_type start = pos;
        pos = serialData.find(";", pos);
        if (pos == std::string::npos) {
          pos = serialData.length();
          look = false;
        }
        parts.push_back(serialData.substr(start, pos - start));
        pos += 1;
      }
    }
  }

  return parts;
}

long long parseDate(const std::string_view &s) {
  long long i;
  auto result = std::from_chars(s.data(), s.data() + s.size(), i);
  if (result.ec == std::errc()) {
    return i;
  } else {
    throw InvalidSerialKeyDate();
  }
}

} // namespace synergy::license
