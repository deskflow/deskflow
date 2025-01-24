/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/FunctionEventJob.h"

//
// FunctionEventJob
//

FunctionEventJob::FunctionEventJob(void (*func)(const Event &, void *), void *arg) : m_func(func), m_arg(arg)
{
  // do nothing
}

FunctionEventJob::~FunctionEventJob()
{
  // do nothing
}

void FunctionEventJob::run(const Event &event)
{
  if (m_func != NULL) {
    m_func(event, m_arg);
  }
}
