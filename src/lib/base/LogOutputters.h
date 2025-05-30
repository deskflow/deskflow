/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/ILogOutputter.h"
#include "common/Common.h"
#include "mt/Thread.h"

#include <QString>
//! Stop traversing log chain outputter
/*!
This outputter performs no output and returns false from \c write(),
causing the logger to stop traversing the outputter chain.  Insert
this to prevent already inserted outputters from writing.
*/
class StopLogOutputter : public ILogOutputter
{
public:
  StopLogOutputter() = default;
  ~StopLogOutputter() override = default;

  // ILogOutputter overrides
  void open(const QString &title) override;
  void close() override;
  void show(bool showIfEmpty) override;
  bool write(LogLevel level, const QString &message) override;
};

//! Write log to console
/*!
This outputter writes output to the console.  The level for each
message is ignored.
*/
class ConsoleLogOutputter : public ILogOutputter
{
public:
  ConsoleLogOutputter() = default;
  ~ConsoleLogOutputter() override = default;

  // ILogOutputter overrides
  void open(const QString &title) override;
  void close() override;
  void show(bool showIfEmpty) override;
  bool write(LogLevel level, const QString &message) override;
  void flush() const;
};

//! Write log to file
/*!
This outputter writes output to the file.  The level for each
message is ignored.
*/

class FileLogOutputter : public ILogOutputter
{
public:
  explicit FileLogOutputter(const QString &logFile);
  ~FileLogOutputter() override = default;

  // ILogOutputter overrides
  void open(const QString &title) override;
  void close() override;
  void show(bool showIfEmpty) override;
  bool write(LogLevel level, const QString &message) override;

  void setLogFilename(const QString &title);

private:
  QString m_fileName;
};

//! Write log to system log
/*!
This outputter writes output to the system log.
*/
class SystemLogOutputter : public ILogOutputter
{
public:
  SystemLogOutputter() = default;
  ~SystemLogOutputter() override = default;

  // ILogOutputter overrides
  void open(const QString &title) override;
  void close() override;
  void show(bool showIfEmpty) override;
  bool write(LogLevel level, const QString &message) override;
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
  SystemLogger(const QString &title, bool blockConsole);
  SystemLogger(SystemLogger const &) = delete;
  SystemLogger(SystemLogger &&) = delete;
  ~SystemLogger();

  SystemLogger &operator=(SystemLogger const &) = delete;
  SystemLogger &operator=(SystemLogger &&) = delete;

private:
  ILogOutputter *m_syslog = nullptr;
  ILogOutputter *m_stop = nullptr;
};
