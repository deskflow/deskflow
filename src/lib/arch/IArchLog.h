/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/LogLevel.h"
#include "common/IInterface.h"

//! Interface for architecture dependent logging
/*!
This interface defines the logging operations required by
deskflow.  Each architecture must implement this interface.
*/
class IArchLog : public IInterface
{
public:
  //! @name manipulators
  //@{

  //! Open the log
  /*!
  Opens the log for writing.  The log must be opened before being
  written to.
  */
  virtual void openLog(const QString &name) = 0;

  //! Close the log
  /*!
  Close the log.
  */
  virtual void closeLog() = 0;

  //! Show the log
  /*!
  Causes the log to become visible.  This generally only makes sense
  for a log in a graphical user interface.  Other implementations
  will do nothing.  Iff \p showIfEmpty is \c false then the implementation
  may optionally only show the log if it's not empty.
  */
  virtual void showLog(bool showIfEmpty) = 0;

  //! Write to the log
  /*!
  Writes the given string to the log with the given level.
  */
  virtual void writeLog(LogLevel, const QString &) = 0;
  //@}
};
