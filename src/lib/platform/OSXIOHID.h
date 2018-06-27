/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

#include <Carbon/Carbon.h>

//! IOHID event on Mac
class OSXIOHID {
public:
    void                postModifierKeys(UInt32 mask);
    void                postKey(const UInt8 virtualKeyCode,
                                const bool down);
    void                fakeMouseButton(UInt32 button, bool press);

private:
    void                postMouseEvent(io_connect_t event, UInt32 type,
                                NXEventData* ev, IOOptionBits flags,
                                IOOptionBits options);
};
