/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2022 Symless Ltd.
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

#include "SerialKey.h"

#include <string>
#include <vector>

class SerialKeyParser {
public:
  virtual bool parse(const std::string &plainSerial);

  const SerialKey &getData() const;
  void setKey(const std::string &key);
  void setType(const std::string &type);
  void setEdition(const std::string &edition);
  void setWarningTime(const std::string &warnTime);
  void setExpirationTime(const std::string &expTime);

private:
  std::string decode(const std::string &serial) const;
  void parseV1(const std::vector<std::string> &parts);
  void parseV2(const std::vector<std::string> &parts);
  std::vector<std::string> splitToParts(const std::string &plainSerial) const;

  SerialKey m_serialKey;
};
