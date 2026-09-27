/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/BaseException.h"

/**
 * @brief The DisplayException class, generic screen exception
 */
class DisplayException : public BaseException
{
  using BaseException::BaseException;
};

/**
 * @brief DisplayOpenFailureException - Thrown when a computer cannot be opened or initialized.
 */
class DisplayOpenFailureException : public DisplayException
{
  using DisplayException::DisplayException;

protected:
  QString getWhat() const throw() override;
};

//! X11 Display unavailable exception
/*!
Thrown when the x11 display cannot be opened or initialized but retrying later
may be successful.
*/
class X11DisplayUnavailableException : public DisplayOpenFailureException
{
public:
  ~X11DisplayUnavailableException() throw() override = default;

  //@}

protected:
  QString getWhat() const throw() override;
};
