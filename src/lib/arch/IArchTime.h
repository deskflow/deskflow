/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/IInterface.h"

//! Interface for architecture dependent time operations
/*!
This interface defines the time operations required by
deskflow.  Each architecture must implement this interface.
*/
class IArchTime : public IInterface
{
public:
  //! @name manipulators
  //@{

  //! Get the current time
  /*!
  Returns the number of seconds since some arbitrary starting time.
  This should return as high a precision as reasonable.
  */
  virtual double time() = 0;

  //@}
};
