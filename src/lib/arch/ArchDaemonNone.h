/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers.
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchDaemon.h"

#define ARCH_DAEMON ArchDaemonNone

//! Dummy implementation of IArchDaemon
/*!
This class implements IArchDaemon for a platform that does not have daemons.
The query functions return false, and \c daemonize() simply calls the passed
function and returns its result.
*/
class ArchDaemonNone : public IArchDaemon
{
public:
  ArchDaemonNone() = default;
  ~ArchDaemonNone() override = default;

  // IArchDaemon overrides
  int daemonize(DaemonFunc const &func) override
  {
    // simply forward the call to func.  obviously, this doesn't
    // do any daemonizing.
    return func();
  }
  QString commandLine() const override
  {
    return {};
  }
};
