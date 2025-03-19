/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <stdexcept>

// apple declares _NOEXCEPT
#ifndef _NOEXCEPT
#define _NOEXCEPT throw()
#endif
