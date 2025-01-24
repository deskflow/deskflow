/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/XBase.h"

//! Generic screen exception
XBASE_SUBCLASS(XScreen, XBase);

//! Cannot open screen exception
/*!
Thrown when a screen cannot be opened or initialized.
*/
XBASE_SUBCLASS_WHAT(XScreenOpenFailure, XScreen);

//! XInput exception
/*!
Thrown when an XInput error occurs
*/
XBASE_SUBCLASS_WHAT(XScreenXInputFailure, XScreen);

//! Screen unavailable exception
/*!
Thrown when a screen cannot be opened or initialized but retrying later
may be successful.
*/
class XScreenUnavailable : public XScreenOpenFailure
{
public:
  /*!
  \c timeUntilRetry is the suggested time the caller should wait until
  trying to open the screen again.
  */
  XScreenUnavailable(double timeUntilRetry);
  virtual ~XScreenUnavailable() _NOEXCEPT;

  //! @name manipulators
  //@{

  //! Get retry time
  /*!
  Returns the suggested time to wait until retrying to open the screen.
  */
  double getRetryTime() const;

  //@}

protected:
  virtual std::string getWhat() const throw();

private:
  double m_timeUntilRetry;
};
