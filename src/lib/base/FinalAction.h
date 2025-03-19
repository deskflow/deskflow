/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <utility>

namespace deskflow {

/**
 * @brief The `FinalAction` class implements a common pattern for calling an action at the end of a function.
 */
template <class Callable> class FinalAction
{
public:
  FinalAction() noexcept
  {
  }

  FinalAction(Callable callable) noexcept : m_callable{callable}
  {
  }

  ~FinalAction() noexcept
  {
    if (!m_invoked) {
      m_callable();
    }
  }

  FinalAction(FinalAction &&other) noexcept : m_callable{std::move(other.m_callable)}
  {
    std::swap(m_invoked, other.m_invoked);
  }

  FinalAction(const FinalAction &) = delete;
  FinalAction &operator=(const FinalAction &) = delete;

private:
  bool m_invoked = false;
  Callable m_callable;
};

template <class Callable> inline FinalAction<Callable> finally(Callable &&callable) noexcept
{
  return FinalAction<Callable>(std::forward<Callable>(callable));
}

} // namespace deskflow
