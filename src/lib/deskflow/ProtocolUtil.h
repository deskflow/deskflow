/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventTypes.h"
#include "io/XIO.h"

#include <stdarg.h>
#include <vector>

namespace deskflow {
class IStream;
}

//! Deskflow protocol utilities
/*!
This class provides various functions for implementing the deskflow
protocol.
*/
class ProtocolUtil
{
public:
  //! Write formatted data
  /*!
  Write formatted binary data to a stream.  \c fmt consists of
  regular characters and format specifiers.  Format specifiers
  begin with \%.  All characters not part of a format specifier
  are regular and are transmitted unchanged.

  Format specifiers are:
  - \%\%   -- literal `\%'
  - \%1i  -- converts integer argument to 1 byte integer
  - \%2i  -- converts integer argument to 2 byte integer in NBO
  - \%4i  -- converts integer argument to 4 byte integer in NBO
  - \%1I  -- converts std::vector<uint8_t>* to 1 byte integers
  - \%2I  -- converts std::vector<uint16_t>* to 2 byte integers in NBO
  - \%4I  -- converts std::vector<uint32_t>* to 4 byte integers in NBO
  - \%s   -- converts std::string* to stream of bytes
  - \%S   -- converts integer N and const uint8_t* to stream of N bytes
  */
  static void writef(deskflow::IStream *, const char *fmt, ...);

  //! Read formatted data
  /*!
  Read formatted binary data from a buffer.  This performs the
  reverse operation of writef().  Returns true if the entire
  format was successfully parsed, false otherwise.

  Format specifiers are:
  - \%\%   -- read (and discard) a literal `\%'
  - \%1i  -- reads a 1 byte integer; argument is a int32_t* or uint32_t*
  - \%2i  -- reads an NBO 2 byte integer;  arg is int32_t* or uint32_t*
  - \%4i  -- reads an NBO 4 byte integer;  arg is int32_t* or uint32_t*
  - \%1I  -- reads 1 byte integers;  arg is std::vector<uint8_t>*
  - \%2I  -- reads NBO 2 byte integers;  arg is std::vector<uint16_t>*
  - \%4I  -- reads NBO 4 byte integers;  arg is std::vector<uint32_t>*
  - \%s   -- reads bytes;  argument must be a std::string*, \b not a char*
  */
  static bool readf(deskflow::IStream *, const char *fmt, ...);

private:
  static void vwritef(deskflow::IStream *, const char *fmt, uint32_t size, va_list);
  static void vreadf(deskflow::IStream *, const char *fmt, va_list);

  static uint32_t getLength(const char *fmt, va_list);
  static void writef(std::vector<uint8_t> &, const char *fmt, va_list);
  static uint32_t eatLength(const char **fmt);
  static void read(deskflow::IStream *, void *, uint32_t);

  /**
   * @brief Handles 1,2, or 4 byte Integers
   */
  static uint8_t read1ByteInt(deskflow::IStream *stream);
  static uint16_t read2BytesInt(deskflow::IStream *stream);
  static uint32_t read4BytesInt(deskflow::IStream *stream);

  /**
   * @brief Handles a Vector of integers
   */
  static void readVector1ByteInt(deskflow::IStream *, std::vector<uint8_t> &);
  static void readVector2BytesInt(deskflow::IStream *, std::vector<uint16_t> &);
  static void readVector4BytesInt(deskflow::IStream *, std::vector<uint32_t> &);
  static uint32_t readVectorSize(deskflow::IStream *stream);

  /**
   * @brief Handles an array of bytes
   */
  static void readBytes(deskflow::IStream *, uint32_t, std::string *);
};

//! Mismatched read exception
/*!
Thrown by ProtocolUtil::readf() when the data being read does not
match the format.
*/
class XIOReadMismatch : public XIO
{
public:
  // XBase overrides
  virtual std::string getWhat() const throw();
};
