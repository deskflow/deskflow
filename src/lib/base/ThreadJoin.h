/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <system_error>
#include <thread>

namespace deskflow {

//! Join a std::thread without ever aborting the process.
/*!
In auto (coordinated) mode, objects that own a worker std::thread are
constructed and destroyed once per role epoch. When a worker exits early
(e.g. a socket it owns was refused) its OS thread id can be recycled by the
platform for a later thread -- including the one now tearing the owner down.
std::thread::join() compares ids and throws
std::system_error(resource_deadlock_would_occur) when they match, reading the
recycled id as a self-join. These joins run in destructors (implicitly
noexcept) or on worker threads whose escaping exceptions terminate the
process, so that throw would kill deskflow on every role switch. Never
self-join, and swallow a join race rather than abort.
*/
inline void joinNoThrow(std::thread &thread) noexcept
{
  if (!thread.joinable()) {
    return;
  }
  if (thread.get_id() == std::this_thread::get_id()) {
    thread.detach();
    return;
  }
  try {
    thread.join();
  } catch (const std::system_error &) {
    thread.detach();
  }
}

} // namespace deskflow
