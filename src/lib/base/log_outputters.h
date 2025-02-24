/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/ILogOutputter.h"
#include "common/common.h"
#include "common/stddeque.h"
#include "mt/Thread.h"

#include <fstream>
#include <list>
#include <string>

//! Stop traversing log chain outputter
/*!
This outputter performs no output and returns false from \c write(),
causing the logger to stop traversing the outputter chain.  Insert
this to prevent already inserted outputters from writing.
*/
class StopLogOutputter : public ILogOutputter
{
public:
  StopLogOutputter();
  virtual ~StopLogOutputter();

  // ILogOutputter overrides
  virtual void open(const char *title);
  virtual void close();
  virtual void show(bool showIfEmpty);
  virtual bool write(ELevel level, const char *message);
};

//! Write log to console
/*!
This outputter writes output to the console.  The level for each
message is ignored.
*/
class ConsoleLogOutputter : public ILogOutputter
{
public:
  ConsoleLogOutputter();
  virtual ~ConsoleLogOutputter();

  // ILogOutputter overrides
  virtual void open(const char *title);
  virtual void close();
  virtual void show(bool showIfEmpty);
  virtual bool write(ELevel level, const char *message);
  virtual void flush();
};

//! Write log to file
/*!
This outputter writes output to the file.  The level for each
message is ignored.
*/

class FileLogOutputter : public ILogOutputter
{
public:
  FileLogOutputter(const char *logFile);
  virtual ~FileLogOutputter();

  // ILogOutputter overrides
  virtual void open(const char *title);
  virtual void close();
  virtual void show(bool showIfEmpty);
  virtual bool write(ELevel level, const char *message);

  void setLogFilename(const char *title);

private:
  std::string m_fileName;
};

//! Write log to system log
/*!
This outputter writes output to the system log.
*/
class SystemLogOutputter : public ILogOutputter
{
public:
  SystemLogOutputter();
  virtual ~SystemLogOutputter();

  // ILogOutputter overrides
  virtual void open(const char *title);
  virtual void close();
  virtual void show(bool showIfEmpty);
  virtual bool write(ELevel level, const char *message);
};

//! Write log to system log only
/*!
Creating an object of this type inserts a StopLogOutputter followed
by a SystemLogOutputter into Log.  The destructor removes those
outputters.  Add one of these to any scope that needs to write to
the system log (only) and restore the old outputters when exiting
the scope.
*/
class SystemLogger
{
public:
  SystemLogger(const char *title, bool blockConsole);
  SystemLogger(SystemLogger const &) = delete;
  SystemLogger(SystemLogger &&) = delete;
  ~SystemLogger();

  SystemLogger &operator=(SystemLogger const &) = delete;
  SystemLogger &operator=(SystemLogger &&) = delete;

private:
  ILogOutputter *m_syslog;
  ILogOutputter *m_stop;
};
