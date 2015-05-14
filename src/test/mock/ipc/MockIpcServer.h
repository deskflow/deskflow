/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
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

#include "ipc/IpcServer.h"
#include "ipc/IpcMessage.h"

#include "test/global/gmock.h"

class IEventQueue;

class MockIpcServer : public IpcServer
{
public:
	MockIpcServer() { }

	MOCK_METHOD0(listen, void());
	MOCK_METHOD2(send, void(const IpcMessage&, EIpcClientType));
	MOCK_CONST_METHOD1(hasClients, bool(EIpcClientType));
};
