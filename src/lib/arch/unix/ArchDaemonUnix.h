/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/ArchDaemonNone.h"

#undef ARCH_DAEMON
#define ARCH_DAEMON ArchDaemonUnix

//! Unix implementation of IArchDaemon
class ArchDaemonUnix : public ArchDaemonNone
{
public:
  ArchDaemonUnix() = default;
  ~ArchDaemonUnix() override = default;

  // IArchDaemon overrides
  int daemonize(const QString &name, DaemonFunc const &func) override;
};
