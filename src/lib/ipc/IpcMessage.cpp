/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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

IpcLogLineMessage::IpcLogLineMessage(const std::string &logLine)
    : IpcMessage(IpcMessageType::LogLine),
      m_logLine(logLine)
{
}

IpcCommandMessage::IpcCommandMessage(const std::string &command, bool elevate)
    : IpcMessage(IpcMessageType::Command),
      m_command(command),
      m_elevate(elevate)
{
}
