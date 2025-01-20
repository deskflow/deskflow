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

#include "FingerprintData.h"
#include "io/filesystem.h"
#include <iosfwd>
#include <string>
#include <vector>

namespace deskflow {

class FingerprintDatabase
{
public:
  void read(const fs::path &path);
  void write(const fs::path &path);

  void read_stream(std::istream &stream);
  void write_stream(std::ostream &stream);

  void clear();
  void add_trusted(const FingerprintData &fingerprint);
  bool is_trusted(const FingerprintData &fingerprint);

  const std::vector<FingerprintData> &fingerprints() const
  {
    return fingerprints_;
  }

  static FingerprintData parse_db_line(const std::string &line);
  static std::string to_db_line(const FingerprintData &fingerprint);

private:
  std::vector<FingerprintData> fingerprints_;
};

} // namespace deskflow
