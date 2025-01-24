/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ArgsBase.h"

namespace deskflow {
enum ClientScrollDirection
{
  SERVER = 1,
  INVERT_SERVER = -1
};

class ClientArgs : public ArgsBase
{

  /// Public functions
public:
  ClientArgs();

  ~ClientArgs() override;

public:
  int m_yscroll = 0;
  bool m_enableLangSync = false; /// @brief Should keyboard input be in same language as on server

  /**
   * @brief m_clientScrollDirection
   * This option is responcible for scroll direction on client side.
   */
  ClientScrollDirection m_clientScrollDirection = ClientScrollDirection::SERVER;

  /**
   * @brief m_hostMode - activates host mode.
   * Client starts a listener and waits for a server connection.
   */
  bool m_hostMode = false;

  /**
   * @brief m_serverAddress stores deskflow server address
   */
  std::string m_serverAddress;
};
} // namespace deskflow
