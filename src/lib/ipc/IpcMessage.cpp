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

#include "ipc/IpcMessage.h"
#include "common/ipc.h"

IpcMessage::IpcMessage(IpcMessageType type) : m_type(type)
{
}

IpcHelloMessage::IpcHelloMessage(IpcClientType clientType) : IpcMessage(IpcMessageType::Hello), m_clientType(clientType)
{
}

IpcHelloBackMessage::IpcHelloBackMessage() : IpcMessage(IpcMessageType::HelloBack)
{
}

IpcShutdownMessage::IpcShutdownMessage() : IpcMessage(IpcMessageType::Shutdown)
{
}

IpcLogLineMessage::IpcLogLineMessage(const String &logLine) : IpcMessage(IpcMessageType::LogLine), m_logLine(logLine)
{
}

IpcCommandMessage::IpcCommandMessage(const String &command, bool elevate)
    : IpcMessage(IpcMessageType::Command),
      m_command(command),
      m_elevate(elevate)
{
}
