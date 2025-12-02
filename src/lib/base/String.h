/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>

//! std::string utilities
/*!
Provides functions for string manipulation.
*/
namespace deskflow::string {

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

//! Convert a string to a size type
/*!
Convert an a \c string to an size type
*/
size_t stringToSizeType(const std::string &string);

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
  static bool less(const std::string_view &a, const std::string_view &b);

  //! Returns true iff \c a is lexicographically equal to \c b
  static bool equal(const std::string &a, const std::string &b);

  //! Returns true iff \c a is lexicographically less than \c b
  static bool cmpLess(const std::string::value_type &a, const std::string::value_type &b);
};

} // namespace deskflow::string
