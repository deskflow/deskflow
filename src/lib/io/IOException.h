/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/BaseException.h"

/**
 * @brief The IOException class Generic i/o exception class
 */
class IOException : public BaseException
{
  using BaseException::BaseException;
};

/**
 * @brief The IOCloseException - Thrown if a stream cannot be closed.
 */
class IOCloseException : public IOException
{
  using IOException::IOException;
};

/**
 * @brief IOClosedException - Thrown when attempting to close or perform I/O on an already closed.
 */
class IOClosedException : public IOException
{
  using IOException::IOException;

protected:
  std::string getWhat() const throw() override;
};

/**
 * @brief IOEndOfStreamException - Thrown when attempting to read beyond the end of a stream.
 */
class IOEndOfStreamException : public IOException
{
  using IOException::IOException;

protected:
  std::string getWhat() const throw() override;
};

/**
 * @brief IOWouldBlockException - Thrown if an operation on a stream would block.
 */
class IOWouldBlockException : public IOException
{
  using IOException::IOException;

protected:
  std::string getWhat() const throw() override;
};
