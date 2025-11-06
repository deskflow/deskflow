/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2018 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include <stdexcept>

#ifdef __APPLE__
#include <string>
#endif

class DisplayInvalidException : public std::runtime_error
{
#ifdef __APPLE__
public:
  explicit DisplayInvalidException(const std::string &msg) : std::runtime_error(msg)
  {
    // do nothing
  }
#endif
};
