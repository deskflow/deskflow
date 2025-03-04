/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2020 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ArgsBase.h"
#include "server/Config.h"

#include <memory>

namespace deskflow {

class ServerArgs : public ArgsBase
{
  using Config = deskflow::server::Config;

public:
  ServerArgs();
  ServerArgs(ServerArgs const &src) = default;
  ServerArgs(ServerArgs &&) = default;
  ~ServerArgs() override;

  ServerArgs &operator=(ServerArgs const &) = default;
  ServerArgs &operator=(ServerArgs &&) = default;

public:
  std::string m_configFile = "";
  std::shared_ptr<Config> m_config;
  bool m_chkPeerCert = true;
};

} // namespace deskflow
