/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

enum qProcessorArch
{
  kProcessorArchWin32,
  kProcessorArchWin64,
  kProcessorArchMac32,
  kProcessorArchMac64,
  kProcessorArchLinux32,
  kProcessorArchLinux64,
  kProcessorArchUnknown
};
