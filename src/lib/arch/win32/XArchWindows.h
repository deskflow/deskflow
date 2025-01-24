/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/XArch.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//! Lazy error message string evaluation for windows
class XArchEvalWindows : public XArchEval
{
public:
  XArchEvalWindows() : m_error(GetLastError())
  {
  }
  XArchEvalWindows(DWORD error) : m_error(error)
  {
  }
  virtual ~XArchEvalWindows()
  {
  }

  virtual std::string eval() const throw();

private:
  DWORD m_error;
};

//! Lazy error message string evaluation for winsock
class XArchEvalWinsock : public XArchEval
{
public:
  XArchEvalWinsock(int error) : m_error(error)
  {
  }
  virtual ~XArchEvalWinsock()
  {
  }

  virtual std::string eval() const throw();

private:
  int m_error;
};
