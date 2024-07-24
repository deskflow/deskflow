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

namespace synergy::license {

using SerialKeyParseError = std::runtime_error;

class InvalidHexString : public SerialKeyParseError {
public:
  explicit InvalidHexString()
      : SerialKeyParseError("Invalid hex string length") {}
};

class InvalidSerialKeyFormat : public SerialKeyParseError {
public:
  explicit InvalidSerialKeyFormat()
      : SerialKeyParseError("Invalid serial key format") {}
};

class InvalidSerialKeyDate : public SerialKeyParseError {
public:
  explicit InvalidSerialKeyDate()
      : SerialKeyParseError("Invalid serial key date") {}
};

class InvalidSerialKeyVersion : public SerialKeyParseError {
public:
  explicit InvalidSerialKeyVersion()
      : SerialKeyParseError("Invalid serial key version") {}
};

SerialKey parseSerialKey(const std::string &hexString);

} // namespace synergy::license
