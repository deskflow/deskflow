/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Bolton Software Ltd.
 * Patch by Ryan Chapman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "platform/OSXClipboardAnyBitmapConverter.h"
#include <algorithm>

COSXClipboardAnyBitmapConverter::COSXClipboardAnyBitmapConverter()
{
	// do nothing
}

COSXClipboardAnyBitmapConverter::~COSXClipboardAnyBitmapConverter()
{
	// do nothing
}

IClipboard::EFormat
COSXClipboardAnyBitmapConverter::getFormat() const
{
	return IClipboard::kBitmap;
}

CString
COSXClipboardAnyBitmapConverter::fromIClipboard(const CString& data) const
{
	return doFromIClipboard(data);
}

CString
COSXClipboardAnyBitmapConverter::toIClipboard(const CString& data) const
{
	return doToIClipboard(data);
}
