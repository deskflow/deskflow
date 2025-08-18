/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

class Event;

//! Use a function as an event job
/*!
An event job class that invokes a function.
*/
class FunctionEventJob
{
public:
  //! run() invokes \c func(arg)
  explicit FunctionEventJob(void (*func)(const Event &, void *), void *arg = nullptr);
  ~FunctionEventJob() = default;
  void run(const Event &);

private:
  void (*m_func)(const Event &, void *);
  void *m_arg;
};
