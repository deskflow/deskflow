/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless Ltd.
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

#include "EditionType.h"
#include <string>

class SerialKeyEdition {
  friend bool operator==(SerialKeyEdition const &,
                         SerialKeyEdition const &) = default;

public:
  SerialKeyEdition() = default;
  explicit SerialKeyEdition(Edition type);
  explicit SerialKeyEdition(const std::string &name);

  Edition getType() const;
  std::string getName() const;
  std::string getProductName() const;

  void setType(Edition type);
  void setType(const std::string &type);

  bool isValid() const;
  bool isChina() const;

  static const std::string Pro;
  static const std::string ProChina;
  static const std::string Basic;
  static const std::string BasicChina;
  static const std::string Buisiness;
  static const std::string Unregistered;
  static const std::string Ultimate;
  static const std::string Lite;

private:
  Edition m_type = kUnregistered;
};
