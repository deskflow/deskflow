/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "IpcMessage.h"
#include <string>

class IpcSettingMessage : public IpcMessage
{
public:
  //!
  //! \brief IpcSettingMessage constructor
  //! \param name - setting name
  //! \param value - setting value
  //!
  IpcSettingMessage(const std::string &name, const std::string &value);

  //!
  //! \brief getName is a getter for the setting name
  //! \return setting name
  //!
  const std::string &getName() const;

  //!
  //! \brief getValue is a getter for the setting value
  //! \return setting value
  //!
  const std::string &getValue() const;

private:
  std::string m_name;
  std::string m_value;
};
