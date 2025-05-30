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
  MSWindowsDebugOutputter() = default;
  ~MSWindowsDebugOutputter() override = default;

  // ILogOutputter overrides
  void open(const QString &title) override;
  void close() override;
  void show(bool showIfEmpty) override;
  bool write(LogLevel level, const QString &message) override;
  void flush();
};
