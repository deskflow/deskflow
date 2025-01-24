/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/ILogOutputter.h"

//! Write log to debugger
/*!
This outputter writes output to the debugger. In Visual Studio, this
can be seen in the Output window.
*/
class MSWindowsDebugOutputter : public ILogOutputter
{
public:
  MSWindowsDebugOutputter();
  virtual ~MSWindowsDebugOutputter();

  // ILogOutputter overrides
  virtual void open(const char *title);
  virtual void close();
  virtual void show(bool showIfEmpty);
  virtual bool write(ELevel level, const char *message);
  virtual void flush();
};
