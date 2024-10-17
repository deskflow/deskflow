/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
