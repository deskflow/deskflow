/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/BaseException.h"

/**
 * @brief The ScreenException class, generic screen exception
 */
class ScreenException : public BaseException
{
  using BaseException::BaseException;
};

/**
 * @brief ScreenOpenFailureException - Thrown when a screen cannot be opened or initialized.
 */
class ScreenOpenFailureException : public ScreenException
{
  using ScreenException::ScreenException;

protected:
  QString getWhat() const throw() override;
};

//! Screen unavailable exception
/*!
Thrown when a screen cannot be opened or initialized but retrying later
may be successful.
*/
class ScreenUnavailableException : public ScreenOpenFailureException
{
public:
  ~ScreenUnavailableException() throw() override = default;

  //@}

protected:
  QString getWhat() const throw() override;
};
