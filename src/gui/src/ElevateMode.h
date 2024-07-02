/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless
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

// The elevate mode tristate determines two behaviours on Windows.
// The first, switch-on-desk-switch (SodS), passed through synergyd as a
// command line argument to synergy core, determines if the server restarts
// when switching Windows desktops (e.g. when Windows UAC dialog pops up).
// The second, passed as a boolean flag to Synergyd over the IPC inside
// kIpcCommandMessage, determines whether Synergy should be started with
// elevated privileges.
//
// The matrix for these two behaviours is as follows:
//                          SodS        Elevate
//                     ___________________________
//  ElevateAsNeeded    |    true    |   false
//  ElevateAlways      |    false   |   true
//  ElevateNever       |    false   |   false
//
enum ElevateMode { ElevateAsNeeded = 0, ElevateAlways = 1, ElevateNever = 2 };

extern const ElevateMode defaultElevateMode;
