/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/unix/ArchStringUnix.h"

#include <stdio.h>

//
// ArchStringUnix
//

#include "arch/multibyte.h"

ArchStringUnix::ArchStringUnix()
{
}

ArchStringUnix::~ArchStringUnix()
{
}

IArchString::EWideCharEncoding ArchStringUnix::getWideCharEncoding()
{
  return kUCS4;
}
