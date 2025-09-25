/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ArgsBase.h"

namespace deskflow {
enum class ClientScrollDirection
{
  Normal = 1,   //!< Same direction as the server
  Inverted = -1 //!< Inverted scroll direction from the server
};

class ClientArgs : public ArgsBase
{

  /// Public functions
public:
  ClientArgs();

  ~ClientArgs() override = default;

public:
  /**
   * @brief m_clientScrollDirection
   * This option is responcible for scroll direction on client side.
   */
  ClientScrollDirection m_clientScrollDirection = ClientScrollDirection::Normal;

  /**
   * @brief m_serverAddress stores deskflow server address
   */
  std::string m_serverAddress;
};
} // namespace deskflow
