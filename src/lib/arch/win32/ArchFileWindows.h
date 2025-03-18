/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchFile.h"

#define ARCH_FILE ArchFileWindows

//! Win32 implementation of IArchFile
class ArchFileWindows : public IArchFile
{
public:
  ArchFileWindows();
  virtual ~ArchFileWindows();

  // IArchFile overrides
  virtual const char *getBasename(const char *pathname);
  virtual std::string getInstalledDirectory();
};
