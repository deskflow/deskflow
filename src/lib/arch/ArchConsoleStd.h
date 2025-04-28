/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchConsole.h"

//! Cross platform implementation of IArchConsole
class ArchConsoleStd : public IArchConsole
{
public:
  ArchConsoleStd()
  {
  }
  ~ArchConsoleStd() override
  {
  }

  // IArchConsole overrides
  void openConsole(const char *title) override
  {
  }
  void closeConsole() override
  {
  }
  void showConsole(bool) override
  {
  }
  void writeConsole(ELevel level, const char *) override;
};
