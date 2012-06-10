/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#include <rfb_win32/TsSessions.h>
#include <rfb_win32/DynamicFn.h>
#include <rfb/LogWriter.h>
#include <rdr/Exception.h>
#include <tchar.h>

#ifdef ERROR_CTX_WINSTATION_BUSY
#define RFB_HAVE_WINSTATION_CONNECT
#else
#pragma message("  NOTE: Not building WinStationConnect support.")
#endif

static rfb::LogWriter vlog("TsSessions");

namespace rfb {
namespace win32 {

  // Windows XP (and later) functions used to handle session Ids
  typedef BOOLEAN (WINAPI *_WinStationConnect_proto) (HANDLE,ULONG,ULONG,PCWSTR,ULONG);
  DynamicFn<_WinStationConnect_proto> _WinStationConnect(_T("winsta.dll"), "WinStationConnectW");
  typedef DWORD (WINAPI *_WTSGetActiveConsoleSessionId_proto) ();
  DynamicFn<_WTSGetActiveConsoleSessionId_proto> _WTSGetActiveConsoleSessionId(_T("kernel32.dll"), "WTSGetActiveConsoleSessionId");
  typedef BOOL (WINAPI *_ProcessIdToSessionId_proto) (DWORD, DWORD*);
  DynamicFn<_ProcessIdToSessionId_proto> _ProcessIdToSessionId(_T("kernel32.dll"), "ProcessIdToSessionId");
  typedef BOOL (WINAPI *_LockWorkStation_proto)();
  DynamicFn<_LockWorkStation_proto> _LockWorkStation(_T("user32.dll"), "LockWorkStation");


  ProcessSessionId::ProcessSessionId(DWORD processId) {
    id = 0;
    if (!_ProcessIdToSessionId.isValid())
      return;
    if (processId == -1)
      processId = GetCurrentProcessId();
    if (!(*_ProcessIdToSessionId)(GetCurrentProcessId(), &id))
      throw rdr::SystemException("ProcessIdToSessionId", GetLastError());
  }

  ProcessSessionId mySessionId;

  ConsoleSessionId::ConsoleSessionId() {
    if (_WTSGetActiveConsoleSessionId.isValid())
      id = (*_WTSGetActiveConsoleSessionId)();
    else
      id = 0;
  }

  bool inConsoleSession() {
    ConsoleSessionId console;
    return console.id == mySessionId.id;
  }

  void setConsoleSession(DWORD sessionId) {
#ifdef RFB_HAVE_WINSTATION_CONNECT
    if (!_WinStationConnect.isValid())
      throw rdr::Exception("WinSta APIs missing");
    if (sessionId == -1)
      sessionId = mySessionId.id;

    // Try to reconnect our session to the console
    ConsoleSessionId console;
    vlog.info("Console session is %d", console.id);
    if (!(*_WinStationConnect)(0, sessionId, console.id, L"", 0))
      throw rdr::SystemException("Unable to connect session to Console", GetLastError());

    // Lock the newly connected session, for security
    if (_LockWorkStation.isValid())
      (*_LockWorkStation)();
#else
    throw rdr::Exception("setConsoleSession not implemented");
#endif
  }

};
};
