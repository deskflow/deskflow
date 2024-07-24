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

#include "Ipc.h"

const char *const kIpcHost = "127.0.0.1";
const int kIpcPort = 24801;

const char *const kIpcMsgHello = "IHEL%1i";
const char *const kIpcMsgHelloBack = "IHEL";
const char *const kIpcMsgLogLine = "ILOG%s";
const char *const kIpcMsgCommand = "ICMD%s%1i";
const char *const kIpcMsgShutdown = "ISDN";
const char *const kIpcMsgSetting = "SSET%s%s";
