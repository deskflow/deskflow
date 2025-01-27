/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/win32/ArchLogWindows.h"
#include "arch/win32/ArchMiscWindows.h"

#include <string.h>

//
// ArchLogWindows
//

ArchLogWindows::ArchLogWindows() : m_eventLog(NULL)
{
  // do nothing
}

ArchLogWindows::~ArchLogWindows()
{
  // do nothing
}

void ArchLogWindows::openLog(const char *name)
{
  if (m_eventLog == NULL) {
    m_eventLog = RegisterEventSource(NULL, name);
  }
}

void ArchLogWindows::closeLog()
{
  if (m_eventLog != NULL) {
    DeregisterEventSource(m_eventLog);
    m_eventLog = NULL;
  }
}

void ArchLogWindows::showLog(bool)
{
  // do nothing
}

void ArchLogWindows::writeLog(ELevel level, const char *msg)
{
  if (m_eventLog != NULL) {
    // convert priority
    WORD type;
    switch (level) {
    case kERROR:
      type = EVENTLOG_ERROR_TYPE;
      break;

    case kWARNING:
      type = EVENTLOG_WARNING_TYPE;
      break;

    default:
      type = EVENTLOG_INFORMATION_TYPE;
      break;
    }

    // log it
    // FIXME -- win32 wants to use a message table to look up event
    // strings.  log messages aren't organized that way so we'll
    // just dump our string into the raw data section of the event
    // so users can at least see the message.  note that we use our
    // level as the event category.
    ReportEvent(
        m_eventLog, type, static_cast<WORD>(level),
        0, // event ID
        NULL, 0,
        (DWORD)strlen(msg) + 1, // raw data size
        NULL,
        const_cast<char *>(msg)
    ); // raw data
  }
}
