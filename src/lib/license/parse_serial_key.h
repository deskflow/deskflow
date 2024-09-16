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

#include "SerialKey.h"

#include <stdexcept>
#include <string>

namespace deskflow::license {

class SerialKeyParseError : public std::runtime_error {
public:
  explicit SerialKeyParseError(const std::string &message)
      : std::runtime_error(message) {}
};

class InvalidHexString : public SerialKeyParseError {
public:
  explicit InvalidHexString() : SerialKeyParseError("invalid hex string") {}
};

class InvalidSerialKeyFormat : public SerialKeyParseError {
public:
  explicit InvalidSerialKeyFormat()
      : SerialKeyParseError("invalid serial key format") {}
};

class InvalidSerialKeyDate : public SerialKeyParseError {
public:
  explicit InvalidSerialKeyDate(
      const std::string &date, const std::string &cause)
      : SerialKeyParseError("invalid serial key date: " + date + "\n" + cause) {
  }
};

class InvalidSerialKeyVersion : public SerialKeyParseError {
public:
  explicit InvalidSerialKeyVersion(const std::string &version)
      : SerialKeyParseError("invalid serial key version: " + version) {}
};

SerialKey parseSerialKey(const std::string &hexString);

} // namespace synergy::license
