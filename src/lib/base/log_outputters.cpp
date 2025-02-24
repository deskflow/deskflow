/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/log_outputters.h"
#include "arch/Arch.h"
#include "base/Path.h"
#include "base/String.h"
#include "base/TMethodJob.h"

#include <fstream>

enum EFileLogOutputter
{
  kFileSizeLimit = 1024 // kb
};

//
// StopLogOutputter
//

StopLogOutputter::StopLogOutputter()
{
  // do nothing
}

StopLogOutputter::~StopLogOutputter()
{
  // do nothing
}

void StopLogOutputter::open(const char *)
{
  // do nothing
}

void StopLogOutputter::close()
{
  // do nothing
}

void StopLogOutputter::show(bool)
{
  // do nothing
}

bool StopLogOutputter::write(ELevel, const char *)
{
  return false;
}

//
// ConsoleLogOutputter
//

ConsoleLogOutputter::ConsoleLogOutputter()
{
}

ConsoleLogOutputter::~ConsoleLogOutputter()
{
}

void ConsoleLogOutputter::open(const char *title)
{
  ARCH->openConsole(title);
}

void ConsoleLogOutputter::close()
{
  ARCH->closeConsole();
}

void ConsoleLogOutputter::show(bool showIfEmpty)
{
  ARCH->showConsole(showIfEmpty);
}

bool ConsoleLogOutputter::write(ELevel level, const char *msg)
{
  ARCH->writeConsole(level, msg);
  return true;
}

void ConsoleLogOutputter::flush()
{
}

//
// SystemLogOutputter
//

SystemLogOutputter::SystemLogOutputter()
{
  // do nothing
}

SystemLogOutputter::~SystemLogOutputter()
{
  // do nothing
}

void SystemLogOutputter::open(const char *title)
{
  ARCH->openLog(title);
}

void SystemLogOutputter::close()
{
  ARCH->closeLog();
}

void SystemLogOutputter::show(bool showIfEmpty)
{
  ARCH->showLog(showIfEmpty);
}

bool SystemLogOutputter::write(ELevel level, const char *msg)
{
  ARCH->writeLog(level, msg);
  return true;
}

//
// SystemLogger
//

SystemLogger::SystemLogger(const char *title, bool blockConsole) : m_stop(NULL)
{
  // redirect log messages
  if (blockConsole) {
    m_stop = new StopLogOutputter; // NOSONAR - Adopted by `Log`
    CLOG->insert(m_stop);
  }
  m_syslog = new SystemLogOutputter; // NOSONAR - Adopted by `Log`
  m_syslog->open(title);
  CLOG->insert(m_syslog);
}

SystemLogger::~SystemLogger()
{
  CLOG->remove(m_syslog);
  delete m_syslog;
  if (m_stop != NULL) {
    CLOG->remove(m_stop);
    delete m_stop;
  }
}

//
// FileLogOutputter
//

FileLogOutputter::FileLogOutputter(const char *logFile)
{
  setLogFilename(logFile);
}

FileLogOutputter::~FileLogOutputter()
{
}

void FileLogOutputter::setLogFilename(const char *logFile)
{
  assert(logFile != NULL);
  m_fileName = logFile;
}

bool FileLogOutputter::write(ELevel level, const char *message)
{
  bool moveFile = false;

  std::ofstream m_handle;
  m_handle.open(deskflow::filesystem::path(m_fileName), std::fstream::app);
  if (m_handle.is_open() && m_handle.fail() != true) {
    m_handle << message << std::endl;

    // when file size exceeds limits, move to 'old log' filename.
    size_t p = m_handle.tellp();
    if (p > (kFileSizeLimit * 1024)) {
      moveFile = true;
    }
  }
  m_handle.close();

  if (moveFile) {
    std::string oldLogFilename = deskflow::string::sprintf("%s.1", m_fileName.c_str());
    remove(oldLogFilename.c_str());
    rename(m_fileName.c_str(), oldLogFilename.c_str());
  }

  return true;
}

void FileLogOutputter::open(const char *title)
{
}

void FileLogOutputter::close()
{
}

void FileLogOutputter::show(bool showIfEmpty)
{
}
