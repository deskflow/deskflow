/*
 * synergy -- mouse and keyboard sharing utility
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

#include "IpcSettingMessage.h"

IpcSettingMessage::IpcSettingMessage(const std::string& name, const std::string& value) :
    IpcMessage(kIpcSetting),
    m_name(name),
    m_value(value)
{

}

const std::string& IpcSettingMessage::getName() const
{
    return m_name;
}

const std::string& IpcSettingMessage::getValue() const
{
    return m_value;
}

