/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/AppUtil.h"

#define ARCH_APP_UTIL AppUtilUnix

class IEventQueue;

class AppUtilUnix : public AppUtil
{
public:
  AppUtilUnix(IEventQueue *events);
  ~AppUtilUnix() override = default;

  int run(int argc, char **argv) override;
  void startNode() override;
  std::vector<std::string> getKeyboardLayoutList() override;
  std::string getCurrentLanguageCode() override;
  std::string m_evdev;
};
