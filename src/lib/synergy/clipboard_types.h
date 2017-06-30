/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "common/basic_types.h"

//! Clipboard ID
/*!
Type to hold a clipboard identifier.
*/
typedef UInt8 ClipboardID;

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
