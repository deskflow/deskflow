/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
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
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#pragma once

#include "ipc/Ipc.h"
#include "base/EventTypes.h"
#include "base/String.h"
#include "base/Event.h"

class IpcMessage : public EventData {
public:
	virtual ~IpcMessage();

	//! Gets the message type ID.
	UInt8				type() const { return m_type; }

protected:
	IpcMessage(UInt8 type);

private:
	UInt8				m_type;
};

class IpcHelloMessage : public IpcMessage {
public:
	IpcHelloMessage(EIpcClientType clientType);
	virtual ~IpcHelloMessage();

	//! Gets the message type ID.
	EIpcClientType			clientType() const { return m_clientType; }

private:
	EIpcClientType			m_clientType;
};

class IpcShutdownMessage : public IpcMessage {
public:
	IpcShutdownMessage();
	virtual ~IpcShutdownMessage();
};


class IpcLogLineMessage : public IpcMessage {
public:
	IpcLogLineMessage(const String& logLine);
	virtual ~IpcLogLineMessage();

	//! Gets the log line.
	String				logLine() const { return m_logLine; }

private:
	String				m_logLine;
};

class IpcCommandMessage : public IpcMessage {
public:
	IpcCommandMessage(const String& command, bool elevate);
	virtual ~IpcCommandMessage();

	//! Gets the command.
	String				command() const { return m_command; }

	//! Gets whether or not the process should be elevated on MS Windows.
	bool				elevate() const { return m_elevate; }

private:
	String				m_command;
	bool				m_elevate;
};
