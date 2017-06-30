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

#pragma once

#define IPC_HOST "127.0.0.1"
#define IPC_PORT 24801

enum EIpcMessage {
    kIpcHello,
    kIpcLogLine,
    kIpcCommand,
    kIpcShutdown,
};

enum EIpcClientType {
    kIpcClientUnknown,
    kIpcClientGui,
    kIpcClientNode,
};

// handshake: node/gui -> daemon
// $1 = type, the client identifies it's self as gui or node (synergyc/s).
extern const char* kIpcMsgHello;

// log line: daemon -> gui
// $1 = aggregate log lines collected from synergys/c or the daemon itself.
extern const char* kIpcMsgLogLine;

// command: gui -> daemon
// $1 = command; the command for the daemon to launch, typically the full
// path to synergys/c. $2 = true when process must be elevated on ms windows.
extern const char* kIpcMsgCommand;

// shutdown: daemon -> node
// the daemon tells synergys/c to shut down gracefully.
extern const char* kIpcMsgShutdown;
