/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/XArch.h"

//! Thread exception to exit
/*!
Thrown by Thread::exit() to exit a thread.  Clients of Thread
must not throw this type but must rethrow it if caught (by
XThreadExit, XThread, or ...).
*/
class XThreadExit : public XThread
{
public:
  //! \c result is the result of the thread
  XThreadExit(void *result) : m_result(result)
  {
  }
  ~XThreadExit()
  {
  }

public:
  void *m_result;
};
