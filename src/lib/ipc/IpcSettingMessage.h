/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
