/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchTime.h"

#define ARCH_TIME ArchTimeUnix

//! Generic Unix implementation of IArchTime
class ArchTimeUnix : public IArchTime
{
public:
  ArchTimeUnix() = default;
  ~ArchTimeUnix() override = default;

  // IArchTime overrides
  double time() override;
};
