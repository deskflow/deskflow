/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/XBase.h"

/**
 * @brief The XIO class Generic i/o exception class
 */
class XIO : public XBase
{
  using XBase::XBase;
};

/**
 * @brief The XIOClose - Thrown if a stream cannot be closed.
 */
class XIOClose : public XIO
{
  using XIO::XIO;
};

/**
 * @brief XIOClosed - Thrown when attempting to close or perform I/O on an already closed.
 */
class XIOClosed : public XIO
{
  using XIO::XIO;

protected:
  std::string getWhat() const throw() override;
};

/**
 * @brief XIOEndOfStream - Thrown when attempting to read beyond the end of a stream.
 */
class XIOEndOfStream : public XIO
{
  using XIO::XIO;

protected:
  std::string getWhat() const throw() override;
};

/**
 * @brief XIOWouldBlock - Thrown if an operation on a stream would block.
 */
class XIOWouldBlock : public XIO
{
  using XIO::XIO;

protected:
  std::string getWhat() const throw() override;
};
