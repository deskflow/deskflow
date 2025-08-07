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
 * @brief MTException generic multithreading exception
 */
class MTException : public XBase
{
  using XBase::XBase;
};

/**
 * @brief MTThreadUnavailableException - Thrown when a thread cannot be created.
 */
class MTThreadUnavailableException : public MTException
{
  using MTException::MTException;

protected:
  std::string getWhat() const throw() override;
};
