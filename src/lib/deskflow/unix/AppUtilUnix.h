/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/AppUtil.h"

#include <memory>

#define ARCH_APP_UTIL AppUtilUnix

class IEventQueue;

#if defined(Q_OS_MACOS)
class MacInputSourceCache;
#endif

class AppUtilUnix : public AppUtil
{
public:
  explicit AppUtilUnix(const IEventQueue *);
  ~AppUtilUnix() override;

  int run() override;
  std::vector<std::string> getKeyboardLayoutList() override;
  std::string getCurrentLanguageCode() override;

private:
#if defined(Q_OS_MACOS)
  std::unique_ptr<MacInputSourceCache> m_inputSourceCache;
#endif
};
