/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace deskflow {

enum FingerprintType
{
  INVALID,
  SHA1, // deprecated
  SHA256,
};

struct FingerprintData
{
  std::string algorithm;
  std::vector<std::uint8_t> data;

  bool valid() const
  {
    return !algorithm.empty();
  }

  bool operator==(const FingerprintData &other) const;
};

const char *fingerprint_type_to_string(FingerprintType type);
FingerprintType fingerprint_type_from_string(const std::string &type);

} // namespace deskflow
