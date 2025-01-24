/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IJob.h"

//! Use a function as a job
/*!
A job class that invokes a function.
*/
class FunctionJob : public IJob
{
public:
  //! run() invokes \c func(arg)
  FunctionJob(void (*func)(void *), void *arg = NULL);
  virtual ~FunctionJob();

  // IJob overrides
  virtual void run();

private:
  void (*m_func)(void *);
  void *m_arg;
};
