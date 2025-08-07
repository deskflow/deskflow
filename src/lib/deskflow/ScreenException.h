/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/XBase.h"

/**
 * @brief The ScreenException class, generic screen exception
 */
class ScreenException : public XBase
{
  using XBase::XBase;
};

/**
 * @brief ScreenOpenFailureException - Thrown when a screen cannot be opened or initialized.
 */
class ScreenOpenFailureException : public ScreenException
{
  using ScreenException::ScreenException;

protected:
  std::string getWhat() const throw() override;
};

//! Screen unavailable exception
/*!
Thrown when a screen cannot be opened or initialized but retrying later
may be successful.
*/
class ScreenUnavailableException : public ScreenOpenFailureException
{
public:
  /*!
  \c timeUntilRetry is the suggested time the caller should wait until
  trying to open the screen again.
  */
  explicit ScreenUnavailableException(double timeUntilRetry);
  ~ScreenUnavailableException() throw() override = default;

  //! @name manipulators
  //@{

  //! Get retry time
  /*!
  Returns the suggested time to wait until retrying to open the screen.
  */
  double getRetryTime() const;

  //@}

protected:
  std::string getWhat() const throw() override;

private:
  double m_timeUntilRetry;
};
