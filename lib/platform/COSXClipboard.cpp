/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "COSXClipboard.h"
#include <Carbon/Carbon.h>

// FIXME -- implement this

//
// COSXClipboard
//

COSXClipboard::COSXClipboard()
{
}

COSXClipboard::~COSXClipboard()
{
}

bool
COSXClipboard::empty()
{
	return true;
}

void
COSXClipboard::add(EFormat, const CString&)
{
}

bool
COSXClipboard::open(Time) const 
{
	return false;
}

void
COSXClipboard::close() const
{
}

IClipboard::Time
COSXClipboard::getTime() const
{
	return 0;
}

bool
COSXClipboard::has(EFormat) const
{
	return false;
}

CString
COSXClipboard::get(EFormat) const
{
	return "";
}
