/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include <utility>

namespace deskflow {

// this implements a common pattern of executing an action at the end of function

template <class Callable> class final_action
{
public:
  final_action() noexcept
  {
  }
  final_action(Callable callable) noexcept : callable_{callable}
  {
  }

  ~final_action() noexcept
  {
    if (!invoked_) {
      callable_();
    }
  }

  final_action(final_action &&other) noexcept : callable_{std::move(other.callable_)}
  {
    std::swap(invoked_, other.invoked_);
  }

  final_action(const final_action &) = delete;
  final_action &operator=(const final_action &) = delete;

private:
  bool invoked_ = false;
  Callable callable_;
};

template <class Callable> inline final_action<Callable> finally(Callable &&callable) noexcept
{
  return final_action<Callable>(std::forward<Callable>(callable));
}

} // namespace deskflow
