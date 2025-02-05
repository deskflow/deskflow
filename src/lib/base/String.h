/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/common.h"
#include <string>

#include <stdarg.h>
#include <vector>

namespace deskflow {

//! std::string utilities
/*!
Provides functions for string manipulation.
*/
namespace string {

//! Format positional arguments
/*!
Format a string using positional arguments.  fmt has literal
characters and conversion specifications introduced by `\%':
- \%\%  -- literal `\%'
- \%{n} -- positional element n, n a positive integer, {} are literal

All arguments in the variable list are const char*.  Positional
elements are indexed from 1.
*/
std::string format(const char *fmt, ...);

//! Format positional arguments
/*!
Same as format() except takes va_list.
*/
std::string vformat(const char *fmt, va_list);

//! Print a string using sprintf-style formatting
/*!
Equivalent to sprintf() except the result is returned as a String.
*/
std::string sprintf(const char *fmt, ...);

//! Find and replace all
/*!
Finds \c find inside \c subject and replaces it with \c replace
*/
void findReplaceAll(std::string &subject, const std::string &find, const std::string &replace);

//! Remove file extension
/*!
Finds the last dot and remove all characters from the dot to the end
*/
std::string removeFileExt(std::string filename);

//! Convert into hexdecimal
/*!
Convert each character in \c subject into hexdecimal form with \c width
Return a new hexString
*/
std::string toHex(const std::string &subject, int width, const char fill = '0');

/**
 * @brief toHex Convert each value in input into a hex string
 * @param input vector of uint8_t
 * @param width
 * @param fill fill character 0 is default
 * @return a hex string
 */
std::string toHex(const std::vector<uint8_t> &input, int width, const char fill = '0');

/**
 * @brief fromHexChar Convert a single char to its hexidecmal value
 * @param c input char 0-F
 * @return The value of c in Hex or -1 for invalid hex chars
 */
int fromHexChar(char c);

/**
 * @brief fromHex Turn the string into a std::vector<uint8_t>
 * @param hexString a Hexidecimal string
 * @return std::vector<uint8_t> version of the hex chars
 */
std::vector<uint8_t> fromHex(const std::string &hexString);

//! Convert to all uppercase
/*!
Convert each character in \c subject to uppercase
*/
void uppercase(std::string &subject);

//! Remove all specific char in suject
/*!
Remove all specific \c c in \c suject
*/
void removeChar(std::string &subject, const char c);

//! Convert a size type to a string
/*!
Convert an size type to a string
*/
std::string sizeTypeToString(size_t n);

//! Convert a string to a size type
/*!
Convert an a \c string to an size type
*/
size_t stringToSizeType(std::string string);

//! Split a string into substrings
/*!
Split a \c string that separated by a \c c into substrings
*/
std::vector<std::string> splitString(std::string string, const char c);

//! Case-insensitive comparisons
/*!
This class provides case-insensitve comparison functions.
*/
class CaselessCmp
{
public:
  //! Same as less()
  bool operator()(const std::string &a, const std::string &b) const;

  //! Returns true iff \c a is lexicographically less than \c b
  static bool less(const std::string &a, const std::string &b);

  //! Returns true iff \c a is lexicographically equal to \c b
  static bool equal(const std::string &a, const std::string &b);

  //! Returns true iff \c a is lexicographically less than \c b
  static bool cmpLess(const std::string::value_type &a, const std::string::value_type &b);

  //! Returns true iff \c a is lexicographically equal to \c b
  static bool cmpEqual(const std::string::value_type &a, const std::string::value_type &b);
};

} // namespace string
} // namespace deskflow
