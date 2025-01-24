/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/unix/XArchUnix.h"

#include <cstring>

//
// XArchEvalUnix
//

std::string XArchEvalUnix::eval() const
{
  // FIXME -- not thread safe
  return strerror(m_error);
}
