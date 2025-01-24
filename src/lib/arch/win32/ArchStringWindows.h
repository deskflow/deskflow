/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchString.h"

#define ARCH_STRING ArchStringWindows

//! Win32 implementation of IArchString
class ArchStringWindows : public IArchString
{
public:
  ArchStringWindows();
  virtual ~ArchStringWindows();

  // IArchString overrides
  virtual EWideCharEncoding getWideCharEncoding();
};
