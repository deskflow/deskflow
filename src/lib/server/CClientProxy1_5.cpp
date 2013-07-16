/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Bolton Software Ltd.
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

#include "CClientProxy1_5.h"
#include "CProtocolUtil.h"
#include "CLog.h"
#include "IStream.h"

//
// CClientProxy1_5
//

CClientProxy1_5::CClientProxy1_5(const CString& name, synergy::IStream* stream, CServer* server, IEventQueue* events) :
	CClientProxy1_4(name, stream, server, events)
{
}

CClientProxy1_5::~CClientProxy1_5()
{
}

void
CClientProxy1_5::fileChunkSending(UInt8 mark, const UInt8* data)
{
	CString chunk(reinterpret_cast<const char*>(data));

	switch (mark) {
	case '0':
		LOG((CLOG_DEBUG2 "file sending start: file size = %s", data));
		break;

	case '1':
		LOG((CLOG_DEBUG2 "file chunk sending: %s", data));
		break;

	case '2':
		LOG((CLOG_DEBUG2 "file sending finished"));
		break;
	}

	CProtocolUtil::writef(getStream(), kMsgDFileTransfer, mark, &chunk);
}
