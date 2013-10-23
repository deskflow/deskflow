/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2012 Nick Bolton
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
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

#include "BasicTypes.h"
#include "CString.h"
#include "Ipc.h"
#include "CEvent.h"

class CIpcMessage : public CEventData {
public:
	virtual ~CIpcMessage();

	//! Gets the message type ID.
	UInt8				type() const { return m_type; }

protected:
	CIpcMessage(UInt8 type);

private:
	UInt8				m_type;
};

class CIpcHelloMessage : public CIpcMessage {
public:
	CIpcHelloMessage(EIpcClientType clientType);
	virtual ~CIpcHelloMessage();

	//! Gets the message type ID.
	EIpcClientType			clientType() const { return m_clientType; }

private:
	EIpcClientType			m_clientType;
};

class CIpcShutdownMessage : public CIpcMessage {
public:
	CIpcShutdownMessage();
	virtual ~CIpcShutdownMessage();
};


class CIpcLogLineMessage : public CIpcMessage {
public:
	CIpcLogLineMessage(const CString& logLine);
	virtual ~CIpcLogLineMessage();

	//! Gets the log line.
	CString				logLine() const { return m_logLine; }

private:
	CString				m_logLine;
};

class CIpcCommandMessage : public CIpcMessage {
public:
	CIpcCommandMessage(const CString& command, bool elevate);
	virtual ~CIpcCommandMessage();

	//! Gets the command.
	CString				command() const { return m_command; }

	//! Gets whether or not the process should be elevated on MS Windows.
	bool				elevate() const { return m_elevate; }

private:
	CString				m_command;
	bool				m_elevate;
};
