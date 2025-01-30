/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <utility>

namespace deskflow {

// this implements a common pattern of executing an action at the end of function

template <class Callable> class FinalAction
{
public:
  FinalAction() noexcept
  {
  }

  FinalAction(Callable callable) noexcept : callable_{callable}
  {
  }

  ~FinalAction() noexcept
  {
    if (!invoked_) {
      callable_();
    }
  }

  FinalAction(FinalAction &&other) noexcept : callable_{std::move(other.callable_)}
  {
    std::swap(invoked_, other.invoked_);
  }

  FinalAction(const FinalAction &) = delete;
  FinalAction &operator=(const FinalAction &) = delete;

private:
  bool invoked_ = false;
  Callable callable_;
};

template <class Callable> inline FinalAction<Callable> finally(Callable &&callable) noexcept
{
  return FinalAction<Callable>(std::forward<Callable>(callable));
}

} // namespace deskflow
