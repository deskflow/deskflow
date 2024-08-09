/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
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

#include "common/basic_types.h"

enum class IpcMessageType : UInt8 {
  Hello,
  HelloBack,
  LogLine,
  Command,
  Shutdown,
  Setting
};

enum class IpcClientType { Unknown, GUI, Node };

const auto kIpcHost = "127.0.0.1";
const auto kIpcPort = 24801;

// handshake: node/gui -> daemon
// $1 = type, the client identifies it's self as gui or node (synergyc/s).
const auto kIpcMsgHello = "IHEL%1i";

// handshake: daemon -> node/gui
// the daemon responds to the handshake.
const auto kIpcMsgHelloBack = "IHEL";

// log line: daemon -> gui
// $1 = aggregate log lines collected from synergys/c or the daemon itself.
const auto kIpcMsgLogLine = "ILOG%s";

// command: gui -> daemon
// $1 = command; the command for the daemon to launch, typically the full
// path to synergys/c. $2 = true when process must be elevated on ms windows.
const auto kIpcMsgCommand = "ICMD%s%1i";

// shutdown: daemon -> node
// the daemon tells synergys/c to shut down gracefully.
const auto kIpcMsgShutdown = "ISDN";

// set setting: gui -> daemon
// $1 = setting name
// $2 = setting value
const auto kIpcMsgSetting = "SSET%s%s";
