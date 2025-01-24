/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchTime.h"

#define ARCH_TIME ArchTimeWindows

//! Win32 implementation of IArchTime
class ArchTimeWindows : public IArchTime
{
public:
  ArchTimeWindows();
  virtual ~ArchTimeWindows();

  // IArchTime overrides
  virtual double time();
};
