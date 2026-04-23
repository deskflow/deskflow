/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/AppUtil.h"

#include <string_view>
#include <vector>

namespace deskflow {

class KeyboardLayoutManager
{
  std::vector<std::string> m_remoteLayouts;
  std::vector<std::string> m_localLayouts;

public:
  explicit KeyboardLayoutManager(
      const std::vector<std::string> &localLayouts = AppUtil::instance().getKeyboardLayoutList()
  );

  void setRemoteLayouts(const std::string_view &remoteLayouts);
  const std::vector<std::string> &getRemoteLayouts() const;
  const std::vector<std::string> &getLocalLayouts() const;
  std::string getMissedLayouts() const;
  std::string getSerializedLocalLayouts() const;
  bool isLayoutInstalled(const std::string &layout) const;
};

} // namespace deskflow
