/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/win32/ArchLogWindows.h"

#include <QString>

//
// ArchLogWindows
//

ArchLogWindows::ArchLogWindows() : m_eventLog(nullptr)
{
  // do nothing
}

void ArchLogWindows::openLog(const QString &name)
{
  if (m_eventLog == nullptr) {
    m_eventLog = RegisterEventSource(nullptr, name.toStdWString().c_str());
  }
}

void ArchLogWindows::closeLog()
{
  if (m_eventLog != nullptr) {
    DeregisterEventSource(m_eventLog);
    m_eventLog = nullptr;
  }
}

void ArchLogWindows::showLog(bool)
{
  // do nothing
}

void ArchLogWindows::writeLog(LogLevel level, const QString &msg)
{
  if (m_eventLog != nullptr) {
    // convert priority
    WORD type;
    switch (level) {
    case LogLevel::Error:
      type = EVENTLOG_ERROR_TYPE;
      break;

    case LogLevel::Warning:
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
        nullptr, 0,
        (DWORD)(msg.length()) + 1, // raw data size
        nullptr,
        const_cast<char *>(msg.toStdString().c_str())
    ); // raw data
  }
}
