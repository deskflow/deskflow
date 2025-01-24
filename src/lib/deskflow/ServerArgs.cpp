/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2020 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerArgs.h"

namespace deskflow {

ServerArgs::~ServerArgs()
{
}

ServerArgs::ServerArgs()
{
  m_classType = kServer;
}
} // namespace deskflow
