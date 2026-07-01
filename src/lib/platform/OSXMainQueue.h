/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <dispatch/dispatch.h>
#include <pthread.h>

#include <type_traits>

namespace deskflow::platform::osx {

/// Runs \p fn on the main dispatch queue when not already on the main thread.
///
/// macOS 14+ asserts the main queue inside Text Input Source (TIS) APIs.
/// deskflow-core runs server/client epochs on a QThread while the GUI main
/// thread sits in QApplication::exec(), so dispatch_sync here does not
/// deadlock: the main queue is free to run the block.
template <typename Fn>
auto runOnMainQueue(Fn &&fn) -> std::invoke_result_t<Fn>
{
  if (pthread_main_np() != 0) {
    return fn();
  }

  using Result = std::invoke_result_t<Fn>;
  if constexpr (std::is_void_v<Result>) {
    dispatch_sync(dispatch_get_main_queue(), ^{ fn(); });
  } else {
    __block Result result;
    dispatch_sync(dispatch_get_main_queue(), ^{ result = fn(); });
    return result;
  }
}

} // namespace deskflow::platform::osx
