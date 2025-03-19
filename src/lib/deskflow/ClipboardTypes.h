/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/common.h"

//! Clipboard ID
/*!
Type to hold a clipboard identifier.
*/
using ClipboardID = uint8_t;

//! @name Clipboard identifiers
//@{
// clipboard identifiers.  kClipboardClipboard is what is normally
// considered the clipboard (e.g. the cut/copy/paste menu items
// affect it).  kClipboardSelection is the selection on those
// platforms that can treat the selection as a clipboard (e.g. X
// windows).  clipboard identifiers must be sequential starting
// at zero.
static const ClipboardID kClipboardClipboard = 0;
static const ClipboardID kClipboardSelection = 1;

// the number of clipboards (i.e. one greater than the last clipboard id)
static const ClipboardID kClipboardEnd = 2;
//@}
