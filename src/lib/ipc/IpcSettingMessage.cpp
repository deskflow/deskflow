/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IpcSettingMessage.h"

IpcSettingMessage::IpcSettingMessage(const std::string &name, const std::string &value)
    : IpcMessage(IpcMessageType::Setting),
      m_name(name),
      m_value(value)
{
}

const std::string &IpcSettingMessage::getName() const
{
  return m_name;
}

const std::string &IpcSettingMessage::getValue() const
{
  return m_value;
}
