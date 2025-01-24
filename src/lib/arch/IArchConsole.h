/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/ELevel.h"
#include "common/IInterface.h"

//! Interface for architecture dependent console output
/*!
This interface defines the console operations required by
deskflow.  Each architecture must implement this interface.
*/
class IArchConsole : public IInterface
{
public:
  //! @name manipulators
  //@{

  //! Open the console
  /*!
  Opens the console for writing.  The console is opened automatically
  on the first write so calling this method is optional.  Uses \c title
  for the console's title if appropriate for the architecture.  Calling
  this method on an already open console must have no effect.
  */
  virtual void openConsole(const char *title) = 0;

  //! Close the console
  /*!
  Close the console.  Calling this method on an already closed console
  must have no effect.
  */
  virtual void closeConsole() = 0;

  //! Show the console
  /*!
  Causes the console to become visible.  This generally only makes sense
  for a console in a graphical user interface.  Other implementations
  will do nothing.  Iff \p showIfEmpty is \c false then the implementation
  may optionally only show the console if it's not empty.
  */
  virtual void showConsole(bool showIfEmpty) = 0;

  //! Write to the console
  /*!
  Writes the given string to the console, opening it if necessary.
  */
  virtual void writeConsole(ELevel, const char *) = 0;

  //@}
};
