/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/FunctionJob.h"

//
// FunctionJob
//

FunctionJob::FunctionJob(void (*func)(void *), void *arg) : m_func(func), m_arg(arg)
{
  // do nothing
}

FunctionJob::~FunctionJob()
{
  // do nothing
}

void FunctionJob::run()
{
  if (m_func != NULL) {
    m_func(m_arg);
  }
}
