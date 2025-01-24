/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2018 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include <stdexcept>
#include <string>

class DisplayInvalidException : public std::runtime_error
{
public:
  DisplayInvalidException(const char *msg) : std::runtime_error(msg)
  {
  }

  DisplayInvalidException(std::string msg) : std::runtime_error(msg)
  {
  }
};
