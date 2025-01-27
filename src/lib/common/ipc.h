/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common.h"

enum class IpcMessageType : uint8_t
{
  Hello,
  HelloBack,
  LogLine,
  Command,
  Shutdown,
  Setting
};

enum class IpcClientType
{
  Unknown,
  GUI,
  Node
};

const auto kIpcHost = "127.0.0.1";
const auto kIpcPort = 24801;

// handshake: node/gui -> daemon
// $1 = type, the client identifies it's self as gui or core (server/client).
const auto kIpcMsgHello = "IHEL%1i";

// handshake: daemon -> node/gui
// the daemon responds to the handshake.
const auto kIpcMsgHelloBack = "IHEL";

// log line: daemon -> gui
// $1 = aggregate log lines collected from core (server/client) or the daemon
// itself.
const auto kIpcMsgLogLine = "ILOG%s";

// command: gui -> daemon
// $1 = command; the command for the daemon to launch, typically the full
// path to core (server/client). $2 = true when process must be elevated on ms
// windows.
const auto kIpcMsgCommand = "ICMD%s%1i";

// shutdown: daemon -> node
// the daemon tells core (server/client) to shut down gracefully.
const auto kIpcMsgShutdown = "ISDN";

// set setting: gui -> daemon
// $1 = setting name
// $2 = setting value
const auto kIpcMsgSetting = "SSET%s%s";
