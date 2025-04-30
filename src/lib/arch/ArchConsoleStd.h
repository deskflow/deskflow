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
  ArchConsoleStd() = default;
  ~ArchConsoleStd() override = default;

  // IArchConsole overrides
  void openConsole(const char *title) override
  {
    // do nothing
  }
  void closeConsole() override
  {
    // do nothing
  }
  void showConsole(bool) override
  {
    // do nothing
  }
  void writeConsole(ELevel level, const char *) override;
};
