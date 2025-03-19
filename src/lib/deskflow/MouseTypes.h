/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/EventTypes.h"

//! Mouse button ID
/*!
Type to hold a mouse button identifier.
*/
using ButtonID = uint8_t;

//! @name Mouse button identifiers
//@{
static const ButtonID kButtonNone = 0;
static const ButtonID kButtonLeft = 1;
static const ButtonID kButtonMiddle = 2;
static const ButtonID kButtonRight = 3;
static const ButtonID kButtonExtra0 = 4;
static const ButtonID kButtonExtra1 = 5;

static const ButtonID kMacButtonRight = 2;
static const ButtonID kMacButtonMiddle = 3;
//@}

static const uint8_t NumButtonIDs = 5;
