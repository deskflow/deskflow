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

#include <optional>
#include <string>
#include <vector>

using Parts = std::vector<std::string>;
using system_clock = std::chrono::system_clock;
using time_point = system_clock::time_point;

namespace synergy::license {

std::string decode(const std::string &hexString);
Parts tokenize(const std::string &plainText);
SerialKey parseV1(const std::string &hexString, const Parts &parts);
SerialKey parseV2(const std::string &hexString, const Parts &parts);
std::optional<time_point> parseDate(const std::string &unixTimeString);

SerialKey parseSerialKey(const std::string &hexString) {
  auto plainText = decode(hexString);
  const auto parts = tokenize(plainText);

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
  if (hexString.length() % 2 != 0) {
    throw InvalidHexString();
  }

  std::string plainText;
  plainText.reserve(hexString.length() / 2);

  for (size_t i = 0; i < hexString.length(); i += 2) {
    std::string byteString = hexString.substr(i, 2);
    auto byte = static_cast<char>(std::stoi(byteString, nullptr, 16));
    plainText.push_back(byte);
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

Parts tokenize(const std::string &plainText) {
  if (plainText.front() != '{' || plainText.back() != '}') {
    throw InvalidSerialKeyFormat();
  }

  const auto serialData = plainText.substr(1, plainText.length() - 2);

  Parts parts;
  std::stringstream ss(serialData);
  std::string item;

  while (std::getline(ss, item, ';')) {
    parts.push_back(item);
  }

  return parts;
}

std::optional<time_point> parseDate(const std::string &unixTimeString) {
  if (unixTimeString.empty()) {
    return std::nullopt;
  }

  auto seconds = std::stoi(unixTimeString);
  if (seconds <= 0) {
    return std::nullopt;
  }

  return time_point{std::chrono::seconds{seconds}};
}

} // namespace synergy::license
