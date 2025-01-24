/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClientArgs.h"

namespace deskflow {

ClientArgs::~ClientArgs()
{
}

ClientArgs::ClientArgs()
{
  m_classType = kClient;
}
} // namespace deskflow
