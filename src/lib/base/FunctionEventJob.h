/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IEventJob.h"

//! Use a function as an event job
/*!
An event job class that invokes a function.
*/
class FunctionEventJob : public IEventJob
{
public:
  //! run() invokes \c func(arg)
  FunctionEventJob(void (*func)(const Event &, void *), void *arg = NULL);
  virtual ~FunctionEventJob();

  // IEventJob overrides
  virtual void run(const Event &);

private:
  void (*m_func)(const Event &, void *);
  void *m_arg;
};
