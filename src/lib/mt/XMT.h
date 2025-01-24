/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/XBase.h"

//! Generic multithreading exception
XBASE_SUBCLASS(XMT, XBase);

//! Thread creation exception
/*!
Thrown when a thread cannot be created.
*/
XBASE_SUBCLASS_WHAT(XMTThreadUnavailable, XMT);
