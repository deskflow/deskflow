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

#include <string>

class SerialKeyType {
private:
  friend bool
  operator==(SerialKeyType const &lhs, SerialKeyType const &rhs) = default;

public:
  static const std::string Trial;
  static const std::string Subscription;

  explicit SerialKeyType() = default;
  explicit SerialKeyType(const std::string_view &type) { setType(type); }

  void setType(const std::string_view &type);
  bool isTrial() const;
  bool isSubscription() const;

private:
  bool m_isTrial = false;
  bool m_isSubscription = false;
};
