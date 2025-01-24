/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/IInterface.h"

//! Interface for architecture dependent sleeping
/*!
This interface defines the sleep operations required by
deskflow.  Each architecture must implement this interface.
*/
class IArchSleep : public IInterface
{
public:
  //! @name manipulators
  //@{

  //! Sleep
  /*!
  Blocks the calling thread for \c timeout seconds.  If
  \c timeout < 0.0 then the call returns immediately.  If \c timeout
  == 0.0 then the calling thread yields the CPU.

  (cancellation point)
  */
  virtual void sleep(double timeout) = 0;

  //@}
};
