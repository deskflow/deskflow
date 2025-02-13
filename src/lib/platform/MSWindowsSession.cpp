/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsSession.h"

#include "arch/win32/XArchWindows.h"
#include "base/Log.h"
#include "deskflow/XDeskflow.h"

#include <Wtsapi32.h>

MSWindowsSession::MSWindowsSession() : m_activeSessionId(-1)
{
}

MSWindowsSession::~MSWindowsSession()
{
}

bool MSWindowsSession::isProcessInSession(const char *name, PHANDLE process = NULL)
{
  // first we need to take a snapshot of the running processes
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    LOG((CLOG_ERR "could not get process snapshot"));
    throw XArch(new XArchEvalWindows());
  }

  PROCESSENTRY32 entry;
  entry.dwSize = sizeof(PROCESSENTRY32);

  // get the first process, and if we can't do that then it's
  // unlikely we can go any further
  BOOL gotEntry = Process32First(snapshot, &entry);
  if (!gotEntry) {
    LOG((CLOG_ERR "could not get first process entry"));
    throw XArch(new XArchEvalWindows());
  }

  // used to record process names for debug info
  std::list<std::string> nameList;

  // now just iterate until we can find winlogon.exe pid
  DWORD pid = 0;
  while (gotEntry) {

    // make sure we're not checking the system process
    if (entry.th32ProcessID != 0) {

      DWORD processSessionId;
      BOOL pidToSidRet = ProcessIdToSessionId(entry.th32ProcessID, &processSessionId);

      if (!pidToSidRet) {
        // if we can not acquire session associated with a specified process,
        // simply ignore it
        LOG(
            (CLOG_DEBUG2 "could not get session id for process: %i %s, code=%i", entry.th32ProcessID, entry.szExeFile,
             GetLastError())
        );
        gotEntry = nextProcessEntry(snapshot, &entry);
        continue;
      } else {
        // only pay attention to processes in the active session
        if (processSessionId == m_activeSessionId) {

          // store the names so we can record them for debug
          nameList.push_back(entry.szExeFile);

          if (_stricmp(entry.szExeFile, name) == 0) {
            pid = entry.th32ProcessID;
          }
        }
      }
    }

    // now move on to the next entry (if we're not at the end)
    gotEntry = nextProcessEntry(snapshot, &entry);
  }

  std::string nameListJoin;
  for (std::list<std::string>::iterator it = nameList.begin(); it != nameList.end(); it++) {
    nameListJoin.append(*it);
    nameListJoin.append(", ");
  }

  LOG((CLOG_DEBUG2 "processes in session %d: %s", m_activeSessionId, nameListJoin.c_str()));

  CloseHandle(snapshot);

  if (pid) {
    if (process != NULL) {
      // now get the process, which we'll use to get the process token.
      LOG((CLOG_DEBUG "found %s in session %i", name, m_activeSessionId));
      *process = OpenProcess(MAXIMUM_ALLOWED, FALSE, pid);
    }
    return true;
  } else {
    LOG((CLOG_DEBUG "did not find %s in session %i", name, m_activeSessionId));
    return false;
  }
}

HANDLE
MSWindowsSession::getUserToken(LPSECURITY_ATTRIBUTES security)
{
  HANDLE sourceToken;
  if (!WTSQueryUserToken(m_activeSessionId, &sourceToken)) {
    LOG((CLOG_ERR "could not get token from session %d", m_activeSessionId));
    throw XArch(new XArchEvalWindows);
  }

  HANDLE newToken;
  if (!DuplicateTokenEx(
          sourceToken, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, security, SecurityImpersonation, TokenPrimary, &newToken
      )) {

    LOG((CLOG_ERR "could not duplicate token"));
    throw XArch(new XArchEvalWindows);
  }

  LOG((CLOG_DEBUG "duplicated, new token: %i", newToken));
  return newToken;
}

BOOL MSWindowsSession::hasChanged()
{
  return (m_activeSessionId != WTSGetActiveConsoleSessionId());
}

void MSWindowsSession::updateActiveSession()
{
  m_activeSessionId = WTSGetActiveConsoleSessionId();
}

BOOL MSWindowsSession::nextProcessEntry(HANDLE snapshot, LPPROCESSENTRY32 entry)
{
  // TODO: issue S3-2021
  // resetting the error state here is acceptable, but having to do so indicates
  // that a different win32 function call has failed beforehand. we should
  // always check for errors after each win32 function call.
  SetLastError(0);

  BOOL gotEntry = Process32Next(snapshot, entry);
  if (!gotEntry) {

    DWORD err = GetLastError();

    // only throw if it's not the end of the snapshot, if not the 'no more
    // files' error then it's probably something serious.
    if (err != ERROR_NO_MORE_FILES) {
      LOG((CLOG_ERR "could not get next process entry"));
      throw XArch(new XArchEvalWindows());
    }
  }

  return gotEntry;
}
